#pragma once 

#include <memory>
#include <string>
#include <mutex>
#include <vector>
#include <unordered_map>

#include <libssh/libssh.h>
#include <libssh/server.h>

#include "TerminalWriter.hpp"

namespace sshGame {

class SSHServer {
    struct UserData {
        int termWidth;
        int termHeight;
        bool allocatedTerminal;
        bool updatedTerm;
    };

    struct SessionData {
        ssh_channel channel;
        bool authenticated;
        std::string username;
        int sessionId;
    };

    public:
        SSHServer(const std::string& bindAddress, const int port, const std::string& rsaKeyPath, const int logLevel = SSH_LOG_NOLOG);
        ~SSHServer();
        
        void bindListen();
        void listenForConnections();
    private:
        static int auth_none(ssh_session session, const char *user, void *userdata);
        static int pty_request(ssh_session session, ssh_channel channel, const char *term, int x, int y, int px, int py, void *userdata);
        static int shell_request(ssh_session session, ssh_channel channel, void *userdata);
        static int exec_request(ssh_session session, ssh_channel channel, const char *command, void *userdata);
        static int window_change(ssh_session session, ssh_channel channel, int width, int height, int pxwidth, int pwheight, void *userdata);
        static ssh_channel new_session_channel(ssh_session session, void *userdata);
        
        void handle_session_connection(ssh_session session);
        void listen_for_messages(SessionData &sessionData, UserData &userData);

        std::mutex m_mtx;
        ssh_bind m_sshBind;
        int m_port;
        uint64_t m_nextSessionID; 

        std::unordered_map<int, std::unique_ptr<TerminalWriter>> m_terminalWriters;
};
}
