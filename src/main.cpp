#include <libssh/libssh.h>
#include <libssh/server.h>
#include <libssh/callbacks.h>
#include <iostream>
#include <cstring>
#include <pthread.h>
#include <pty.h>
#include <poll.h>
#include <utmp.h>
#include <unistd.h>
#include <thread>
#include <chrono>

const char* hostkey_path = "ssh_host_rsa_key";

const int PORT = 2200;
int authenticated = 0;
int allocatedTerminal = 0;
static int auth_none(ssh_session session,
                     const char *user,
                     void *userdata)
{
    authenticated = 1;
    return SSH_AUTH_SUCCESS;
}

static ssh_channel chan=NULL;

struct userdata {
    int termWidth; 
    int termHeight;
};

struct userdata user; 

static int pty_request(ssh_session session, ssh_channel channel, const char *term, int x, int y, int px, int py, void *userdata){
    user.termWidth = x;
    user.termHeight = y;
    std::cout << "Term width: " << x << std::endl;
    std::cout << "Term height: " << y << std::endl;
    printf("Allocated terminal\n");
    allocatedTerminal = 1;
    return 0;
}

static int shell_request(ssh_session session, ssh_channel channel, void *userdata){
    printf("Allocated shell\n");
    return 0;
}

static int exec_request(ssh_session session, ssh_channel channel, const char *command, void *userdata) {
    return 0;
}
static int window_change(ssh_session session, ssh_channel channel, int width, int height, int pxwidth, int pwheight, void *userdata) {
    user.termWidth = width;
    user.termHeight = height;
 
    std::cout << "Term size changed: " << width << ", " << height << std::endl;
    return 0;
}
struct ssh_channel_callbacks_struct channel_cb = {
    .channel_pty_request_function = pty_request,
    .channel_shell_request_function = shell_request,
    .channel_pty_window_change_function = window_change,
    .channel_exec_request_function = exec_request,
};

static ssh_channel new_session_channel(ssh_session session, void *userdata){
    if(chan != NULL)
        return NULL;
    printf("Allocated session channel\n");
    chan = ssh_channel_new(session);
    ssh_callbacks_init(&channel_cb);
    ssh_set_channel_callbacks(chan, &channel_cb);
    return chan;
}

void moveCursorToMiddle(ssh_channel chan) {
    int middleX = user.termWidth/ 2;
    int middleY = user.termHeight / 2;
    
    std::cout << "Writing at: " << middleX << ", " << middleY << std::endl;
    std::string cursorPosition = "\e[" + std::to_string(middleY) + ";" + std::to_string(middleX) + "H";
    
    if (ssh_channel_write(chan, cursorPosition.c_str(), cursorPosition.size()) == SSH_ERROR) {
        printf("Error moving cursor\n");
        return;
    }
}
void typeHelloWorldWithDelay(ssh_channel chan) {
    std::string message = "Hello World";
    
    for (char c : message) {
        if (ssh_channel_write(chan, &c, 1) == SSH_ERROR) {
            printf("Error writing to channel\n");
            return;
        }

        fflush(stdout);

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}
static void handle_session(ssh_event event, ssh_session session) {
}
int main() {
    struct ssh_server_callbacks_struct cb = {
        .userdata = NULL,
        .auth_none_function = auth_none,
        .channel_open_request_session_function = new_session_channel,
    };
    //ssh_set_log_level(SSH_LOG_FUNCTIONS);
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

    ssh_session session = ssh_new();
    if (ssh_bind_accept(sshbind, session) != SSH_OK) {
        std::cerr << "Error accepting connection: " << ssh_get_error(sshbind) << std::endl;
        ssh_free(session);
        return 1;
    }

    ssh_callbacks_init(&cb);
    ssh_set_server_callbacks(session, &cb);
    std::cout << "Client connected." << std::endl;

    if (ssh_handle_key_exchange(session)) {
        printf("ssh_handle_key_exchange: %s\n", ssh_get_error(session));
        return 1;
    }
    ssh_event mainloop = ssh_event_new();
    ssh_event_add_session(mainloop, session);
    int i;
    int r;
    char buf[2049];
    static int error;

    while (!(authenticated && chan != NULL && allocatedTerminal)){
        if(error)
            break;
        r = ssh_event_dopoll(mainloop, -1);
        if (r == SSH_ERROR){
            printf("Error : %s\n",ssh_get_error(session));
            ssh_disconnect(session);
            return 1;
        }
    }
    if(error){
        printf("Error, exiting loop\n");
    } else {
        printf("Authenticated and got a channel\n");
        std::string altBuf = "\e[?1049h\e[?25l";

        if (ssh_channel_write(chan, altBuf.c_str(), altBuf.size()) == SSH_ERROR) {
            printf("error writing to channel\n");
            return 1;
        }
        moveCursorToMiddle(chan);
        typeHelloWorldWithDelay(chan);
        do{
            i=ssh_channel_read(chan, buf, sizeof(buf) - 1, 0);
            if(i>0) {
                if (buf[0] == '\003') {
                    std::string altBufDisable = "\e[?1049l\e[?25h";
                    if (ssh_channel_write(chan, altBufDisable.c_str(), altBufDisable.size()) == SSH_ERROR) {
                        printf("error writing to channel\n");
                        return 1;
                    }
                    break; 
                } 

                buf[i] = '\0';
                printf("%s", buf);
                fflush(stdout);
                if (buf[0] == '\x0d') {
                    if (ssh_channel_write(chan, "\n", 1) == SSH_ERROR) {
                        printf("error writing to channel\n");
                        return 1;
                    }

                    printf("\n");
                }
            }
        } while (i>0);
    }
    ssh_disconnect(session);
    ssh_bind_free(sshbind);
    ssh_finalize();

    std::cout << "Client disconnected." << std::endl;

    return 0;
}

