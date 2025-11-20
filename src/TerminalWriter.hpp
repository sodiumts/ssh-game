#pragma once

#include "ANSIImage.hpp"
#include <libssh/libssh.h>
#include <string>

#define MIN_WIDTH 100
#define MIN_HEIGHT 40

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
	    void enable_mouse_reporting();

        std::string get_centered_text(const std::string &text);
        void clear_buffer();   
        void write_buffer(const std::string &buffer);

        void update_terminal(int width, int height);
        void write_image(ANSIImage &image, bool centered = false);
            
        std::tuple<int, int, int> get_color_components(const std::string &color);
        std::string print_text(const std::string &text, int x = 0, int y = 0, const std::string &fg_color = "#FFFFFF", const std::string &bg_color = "#000000");
        
        std::string get_buffer();
    private: 
        std::string print_at_position(int x, int y, const std::string &utf_char, const std::string& fg_color, const std::string& bg_color);
        ssh_channel &m_channel;
        int m_width;
        int m_height;
        std::string m_screenBuffer;
};
}
