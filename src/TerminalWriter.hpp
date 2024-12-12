#pragma once

#include <libssh/libssh.h>
#include <string>

namespace sshGame {
class TerminalWriter {
    public:
        TerminalWriter(ssh_channel &channel);
        ~TerminalWriter();
        
        TerminalWriter(const TerminalWriter&) = delete;
        TerminalWriter& operator=(const TerminalWriter&) = delete;
        
        void write_string(const std::string &text);

        void alternate_screen_buffer_enable();
        void alternate_screen_buffer_disable();

        void enable_cursor();
        void disable_cursor();
    private: 
        ssh_channel &m_channel;
};
}
