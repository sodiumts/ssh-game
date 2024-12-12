#include <libssh/libssh.h>
#include <libssh/server.h>
#include <libssh/callbacks.h>
#include <iostream>
#include <cstring>
#include <string>
#include <thread>
#include <chrono>

const char* hostkey_path = "ssh_host_rsa_key";
const int PORT = 2200;

struct UserData {
    int termWidth; 
    int termHeight;
    bool allocatedTerminal;
};

struct SessionData {
    ssh_channel channel;
    bool authenticated;
};

static int auth_none(ssh_session session, const char *user, void *userdata) {
    struct SessionData* sdata = static_cast<SessionData*>(userdata);
    sdata->authenticated = 1;
    std::cout << "Successful authentication" << std::endl;
    return SSH_AUTH_SUCCESS;
}

static int pty_request(ssh_session session, ssh_channel channel, const char *term, int x, int y, int px, int py, void *userdata){
    struct UserData *udata = static_cast<UserData*>(userdata);
    std::cout << "Pty request made" << std::endl;
    udata->termWidth = x;
    udata->termHeight = y;
    udata->allocatedTerminal = true;

    std::cout << "Term width: " << x << std::endl;
    std::cout << "Term height: " << y << std::endl;
    std::cout << "Allocated pty" << std::endl;
    return 0;
}

static int shell_request(ssh_session session, ssh_channel channel, void *userdata){
    std::cout << "Allocated Shell" << std::endl;
    return 0;
}

static int exec_request(ssh_session session, ssh_channel channel, const char *command, void *userdata) {
    std::cout << "Exec request made" << std::endl;
    return 0;
}

static int window_change(ssh_session session, ssh_channel channel, int width, int height, int pxwidth, int pwheight, void *userdata) {
    std::cout << "Window change made" << std::endl;
    struct UserData *udata = static_cast<UserData*>(userdata);
    udata->termWidth = width;
    udata->termHeight = height;
 
    std::cout << "Term size changed: " << width << ", " << height << std::endl;
    return 0;
}

static ssh_channel new_session_channel(ssh_session session, void *userdata){
    struct SessionData* sdata = static_cast<SessionData*>(userdata);
    if(sdata->channel != nullptr)
        return nullptr;
    std::cout << "Allocated session channel" << std::endl;
    sdata->channel = ssh_channel_new(session);
    return sdata->channel;
}

void moveCursorToMiddle(ssh_channel chan, UserData *user) {
    int middleX = user->termWidth/ 2;
    int middleY = user->termHeight / 2;
    
    std::cout << "Writing at: " << middleX << ", " << middleY << std::endl;
    std::string cursorPosition = "\e[" + std::to_string(middleY) + ";" + std::to_string(middleX) + "H";
    
    if (ssh_channel_write(chan, cursorPosition.c_str(), cursorPosition.size()) == SSH_ERROR) {
        std::cerr << "Error moving cursor\n" << std::endl;
        return;
    }
}

void typeHelloWorldWithDelay(ssh_channel chan) {
    std::string message = "Hello World";
    
    for (char c : message) {
        if (ssh_channel_write(chan, &c, 1) == SSH_ERROR) {
            std::cout << "Error writing Hello World to channel" << std::endl;
            return;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

static void handle_session(ssh_session session) {
    struct UserData udata = {0, 0, false};
    struct SessionData sdata = {nullptr, false};

    struct ssh_server_callbacks_struct server_cb = {
        .userdata = &sdata,
        .auth_none_function = auth_none,
        .channel_open_request_session_function = new_session_channel,
    };

    struct ssh_channel_callbacks_struct channel_cb = {
        .userdata = &udata,
        .channel_pty_request_function = pty_request,
        .channel_shell_request_function = shell_request,
        .channel_pty_window_change_function = window_change,
        .channel_exec_request_function = exec_request,
    };

    ssh_callbacks_init(&server_cb);
    ssh_set_server_callbacks(session, &server_cb);
    ssh_callbacks_init(&channel_cb);
    std::cout << "Client Connected" << std::endl;
    
    
    if (ssh_handle_key_exchange(session)) {
        std::cout << "ssh_handle_key_exchange: " << ssh_get_error(session) << std::endl;
        ssh_disconnect(session);
        return;
    }
    std::cout << "Successful key exchange" << std::endl;

    ssh_event mainLoop = ssh_event_new();
    ssh_event_add_session(mainLoop, session);
    
    std::cout << "Going into main loop" << std::endl;
    while (!(sdata.authenticated && sdata.channel != NULL)){
        int r = ssh_event_dopoll(mainLoop, -1);
        if (r == SSH_ERROR){
            printf("Error : %s\n",ssh_get_error(session));
            ssh_disconnect(session);
            return;
        }
    }
    
    ssh_set_channel_callbacks(sdata.channel, &channel_cb);
    
    while (!udata.allocatedTerminal) {
        if (ssh_event_dopoll(mainLoop, -1) == SSH_ERROR){
            printf("Error : %s\n",ssh_get_error(session));
            ssh_disconnect(session);
            return;
        } 
    }
    
    // initial second buffer setup 
    printf("Authenticated and got a channel\n");
    std::string altBuf = "\e[?1049h\e[?25l";

    if (ssh_channel_write(sdata.channel, altBuf.c_str(), altBuf.size()) == SSH_ERROR) {
        std::cout << "Error writing to channel" << std::endl;
        ssh_channel_free(sdata.channel);
        ssh_disconnect(session);
        return;
    }

    
    moveCursorToMiddle(sdata.channel, &udata);
    typeHelloWorldWithDelay(sdata.channel);
    
    int i;
    do{
        char buf[2049];
        i=ssh_channel_read(sdata.channel, buf, sizeof(buf) - 1, 0);
        if(i>0) {
            if (buf[0] == '\x03' || buf[0] == '\x04') { // handle ctrl+c and ctrl+d
                std::string altBufDisable = "\e[?1049l\e[?25h";
                if (ssh_channel_write(sdata.channel, altBufDisable.c_str(), altBufDisable.size()) == SSH_ERROR) {
                    printf("error writing to channel\n");
                    continue;
                }
                break; 
            } 

            buf[i] = '\0';
            std::cout << buf<< std::endl;
        }
    } while (i>0);

    std::cout << "Client disconnected" << std::endl;
    ssh_channel_free(sdata.channel);
    ssh_disconnect(session);
}

int main() {
    ssh_set_log_level(SSH_LOG_FUNCTIONS);
    ssh_bind sshbind = ssh_bind_new();
    ssh_bind_options_set(sshbind, SSH_BIND_OPTIONS_BINDADDR, "0.0.0.0");
    ssh_bind_options_set(sshbind, SSH_BIND_OPTIONS_RSAKEY, hostkey_path);
    ssh_bind_options_set(sshbind, SSH_BIND_OPTIONS_BINDPORT, &PORT);

    if (ssh_bind_listen(sshbind) != SSH_OK) {
        std::cerr << "Error binding SSH server: " << ssh_get_error(sshbind) << std::endl;
        ssh_bind_free(sshbind);
        return 1;
    }

    std::cout << "SSH server started. Waiting for connections..." << std::endl;
    while(true) {
        ssh_session session = ssh_new();
        if(ssh_bind_accept(sshbind, session) == SSH_ERROR) {
            std::cerr << "Issue binding and accepting" << std::endl;
            continue;
        }

        std::thread(handle_session, session).detach();

    }

    ssh_bind_free(sshbind);
    ssh_finalize();
    return 0;
}

