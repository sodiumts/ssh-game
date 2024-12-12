#include "SSHServer.hpp"

#include <array>
#include <stdexcept>
#include <format>
#include <thread>
#include <print>

#include <libssh/callbacks.h>

namespace sshGame {
SSHServer::SSHServer(const std::string& bindAddress, const int port, const std::string& rsaKeyPath, const int logLevel) : m_port(port) {
    ssh_set_log_level(logLevel);
    m_sshBind = ssh_bind_new();
    ssh_bind_options_set(m_sshBind, SSH_BIND_OPTIONS_BINDADDR, bindAddress.c_str());
    ssh_bind_options_set(m_sshBind, SSH_BIND_OPTIONS_RSAKEY, rsaKeyPath.c_str());
    ssh_bind_options_set(m_sshBind, SSH_BIND_OPTIONS_BINDPORT, &port); 
}

SSHServer::~SSHServer() {
    ssh_bind_free(m_sshBind);
    ssh_finalize();
}

void SSHServer::bindListen() {
    if (ssh_bind_listen(m_sshBind) == SSH_ERROR) {
        std::string error = std::format("Error binding SSH server: {}", ssh_get_error(m_sshBind));
        throw std::runtime_error(error);
    }   
    std::println("Server is listening for connections on port {}", m_port);
}

void SSHServer::listenForConnections() {
    while(true) {
        ssh_session session = ssh_new();
        if (ssh_bind_accept(m_sshBind, session) == SSH_ERROR) {
            continue;
        }

        std::thread([this, session]() {
            handle_session_connection(session);
        }).detach(); // We want the threads to independently stop and get freed up
    }
}

int SSHServer::auth_none(ssh_session session, const char *user, void *userdata) {
    struct SessionData* sdata = static_cast<SessionData*>(userdata);
    sdata->authenticated = 1;
    sdata->username = user;
    return SSH_AUTH_SUCCESS;
}

int SSHServer::pty_request(ssh_session session, ssh_channel channel, const char *term, int x, int y, int px, int py, void *userdata) {
    struct UserData *udata = static_cast<UserData*>(userdata);
    udata->termWidth = x;
    udata->termHeight = y;
    udata->allocatedTerminal = true;
    return 0;
}

int SSHServer::shell_request(ssh_session session, ssh_channel channel, void *userdata) {
    return 0;
}

int SSHServer::exec_request(ssh_session session, ssh_channel channel, const char *command, void *userdata) {
    return 0;
}

int SSHServer::window_change(ssh_session session, ssh_channel channel, int width, int height, int pxwidth, int pwheight, void *userdata) {
    struct UserData *udata = static_cast<UserData*>(userdata);
    udata->termWidth = width;
    udata->termHeight = height; 
    return 0;
}

ssh_channel SSHServer::new_session_channel(ssh_session session, void *userdata) {
    struct SessionData* sdata = static_cast<SessionData*>(userdata);
    if(sdata->channel != nullptr)
        return nullptr;
    sdata->channel = ssh_channel_new(session);
    return sdata->channel;
}


void SSHServer::handle_session_connection(ssh_session session) {
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
    
    
    if (ssh_handle_key_exchange(session)) {
        ssh_disconnect(session);
        return;
    }

    ssh_event mainLoop = ssh_event_new();
    ssh_event_add_session(mainLoop, session);
    
    while (!(sdata.authenticated && sdata.channel != nullptr)){
        if (ssh_event_dopoll(mainLoop, -1) == SSH_ERROR){
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
    
    std::println("User: {} connected", sdata.username);
    
    m_mtx.lock();
    m_terminalWriters[m_nextSessionID] = std::move(std::make_unique<TerminalWriter>(sdata.channel));    
    sdata.sessionId = m_nextSessionID;
    m_nextSessionID += 1;
    m_mtx.unlock();

    auto &terminalWriter = m_terminalWriters[sdata.sessionId];
    terminalWriter->disable_cursor();
    terminalWriter->alternate_screen_buffer_enable();

    listen_for_messages(sdata);
    
    std::println("User: {} disconnected", sdata.username);
    
    m_mtx.lock();
    m_terminalWriters.erase(sdata.sessionId);
    m_mtx.unlock();

    ssh_channel_free(sdata.channel);
    ssh_disconnect(session);
}

void SSHServer::listen_for_messages(SessionData &sessionData) {
    auto &terminalWriter = m_terminalWriters[sessionData.sessionId];

    std::array<char, 2049> buffer;
    int i;
    do {
        i=ssh_channel_read(sessionData.channel, buffer.data(), buffer.size() - 1, 0);
        if (i > 0) {
            if (buffer[0] == '\x03' || buffer[0] == '\x04') { // handle ctrl+c and ctrl+d
                terminalWriter->alternate_screen_buffer_disable();
                terminalWriter->enable_cursor();
                break; 
            } 

            buffer[i] = '\0';
            std::println("Message from {}: {}", sessionData.username, buffer.data());
        }
    } while (i > 0); 
}
};

