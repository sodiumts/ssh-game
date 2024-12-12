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
const char* hostkey_path = "ssh_host_rsa_key";

const int PORT = 2200;
int authenticated = 0;

static int auth_none(ssh_session session,
                     const char *user,
                     void *userdata)
{

    (void)user; /* unused */
    (void)userdata; /* unused */
    authenticated = 1;
    return SSH_AUTH_SUCCESS;
}

static ssh_channel chan=NULL;

static int pty_request(ssh_session session, ssh_channel channel, const char *term,
        int x,int y, int px, int py, void *userdata){
    (void) session;
    (void) channel;
    (void) term;
    (void) x;
    (void) y;
    (void) px;
    (void) py;
    (void) userdata;
    printf("Allocated terminal\n");
    return 0;
}

static int shell_request(ssh_session session, ssh_channel channel, void *userdata){
    (void)session;
    (void)channel;
    (void)userdata;
    printf("Allocated shell\n");
    return 0;
}
struct ssh_channel_callbacks_struct channel_cb = {
    .channel_pty_request_function = pty_request,
    .channel_shell_request_function = shell_request
};

static ssh_channel new_session_channel(ssh_session session, void *userdata){
    (void) session;
    (void) userdata;
    if(chan != NULL)
        return NULL;
    printf("Allocated session channel\n");
    chan = ssh_channel_new(session);
    ssh_callbacks_init(&channel_cb);
    ssh_set_channel_callbacks(chan, &channel_cb);
    return chan;
}
int main() {

    struct ssh_server_callbacks_struct cb = {
        .userdata = NULL,
        .auth_none_function = auth_none,
        .channel_open_request_session_function = new_session_channel
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

    while (!(authenticated && chan != NULL)){
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
        std::string altBuf = "\e[?1049h";

        if (ssh_channel_write(chan, altBuf.c_str(), altBuf.size()) == SSH_ERROR) {
            printf("error writing to channel\n");
            return 1;
        }
        do{
            i=ssh_channel_read(chan, buf, sizeof(buf) - 1, 0);
            if(i>0) {
                if (buf[0] == '\003') {
                    std::string altBufDisable = "\e[?1049l";
                    if (ssh_channel_write(chan, altBufDisable.c_str(), altBufDisable.size()) == SSH_ERROR) {
                        printf("error writing to channel\n");
                        return 1;
                    }
                    break; 
                } 
                // if (ssh_channel_write(chan, buf, i) == SSH_ERROR) {
                //     printf("error writing to channel\n");
                //     return 1;
                // }

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

    ssh_bind_free(sshbind);
    return 0;
}

