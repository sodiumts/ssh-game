#pragma once

#include <libssh/libssh.h>
#include <string>

namespace sshGame {
class TerminalWriter {
    public:
        TerminalWriter(ssh_channel &channel, int width, int height);
        ~TerminalWriter();
        
        TerminalWriter(const TerminalWriter&) = delete;
        TerminalWriter& operator=(const TerminalWriter&) = delete;
        
        void write_string(const std::string &text);

        void alternate_screen_buffer_enable();
        void alternate_screen_buffer_disable();

        void enable_cursor();
        void disable_cursor();
        void clear_screen();
        void update_terminal(int width, int height);
        void write_image(const std::string &image);
        void print_at_position(int x, int y, const std::string &utf_char, const std::string& fg_color, const std::string& bg_color);
    private: 
        ssh_channel &m_channel;
        int m_width;
        int m_height;
};
}
