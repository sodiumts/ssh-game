#include "TerminalWriter.hpp"
#include "ANSIImage.hpp"

#include <vector>

namespace sshGame {

TerminalWriter::TerminalWriter(ssh_channel &channel, int width, int height) : m_channel(channel), m_width(width), m_height(height), m_screenBuffer("") { }

TerminalWriter::~TerminalWriter() {}

void TerminalWriter::write_string(const std::string &text) {
    ssh_channel_write(m_channel, text.c_str(), text.size());
}

void TerminalWriter::alternate_screen_buffer_enable() {
    write_string("\e[?1049h");
}

void TerminalWriter::alternate_screen_buffer_disable() {
    write_string("\e[?1049l");
}

void TerminalWriter::clear_screen() {
    write_string("\033[2J");
}

void TerminalWriter::enable_cursor() {
    write_string("\e[?25h");
}

void TerminalWriter::disable_cursor() {
    write_string("\e[?25l");
}

std::string TerminalWriter::get_buffer() {
    return m_screenBuffer;
}

void TerminalWriter::clear_buffer() {
    m_screenBuffer = "";
}
void TerminalWriter::enable_mouse_reporting() {
    write_string("\e[?1000h");
}

std::string TerminalWriter::get_centered_text(const std::string& text) {
    int xPos = (m_width - text.length()) / 2;
    int yPos = m_height / 2;

    std::string result = "\033[" + std::to_string(yPos) + ";" + std::to_string(xPos + 1) + "H" + text;
    
    return result;
}

void TerminalWriter::update_terminal(int width, int height) {
    m_width = width;
    m_height = height; 
}

void TerminalWriter::write_buffer(const std::string &buffer) {
    m_screenBuffer = buffer;
    write_string(buffer);
}

std::tuple<int, int, int> TerminalWriter::get_color_components(const std::string &color) {
    int r = std::stoi(color.substr(1, 2), nullptr, 16);
    int g = std::stoi(color.substr(3, 2), nullptr, 16);
    int b = std::stoi(color.substr(5, 2), nullptr, 16);
    return {r, g, b};
}

std::string TerminalWriter::print_at_position(int x, int y, const std::string& utf_char, const std::string& fg_color, const std::string& bg_color) {
    auto [fg_r, fg_g, fg_b] = get_color_components(fg_color);
    auto [bg_r, bg_g, bg_b] = get_color_components(bg_color);
    
    std::string print = "\033[" + std::to_string(y) + ";" + std::to_string(x) + "H";
    print += "\033[38;2;" + std::to_string(fg_r) + ";" + std::to_string(fg_g) + ";" + std::to_string(fg_b) + "m";
    
    if(bg_r != 0 && bg_g != 0 && bg_b != 0)
        print += "\033[48;2;" + std::to_string(bg_r) + ";" + std::to_string(bg_g) + ";" + std::to_string(bg_b) + "m";
    print += utf_char;
    print += "\033[0m";

    return print;
}

std::string TerminalWriter::print_text(const std::string &text, int x, int y, const std::string &fg_color, const std::string &bg_color) {
    auto [fg_r, fg_g, fg_b] = get_color_components(fg_color);
    auto [bg_r, bg_g, bg_b] = get_color_components(bg_color);
    std::string print = "";
    print = "\033[" + std::to_string(y) + ";" + std::to_string(x) + "H";
    print += "\033[38;2;" + std::to_string(fg_r) + ";" + std::to_string(fg_g) + ";" + std::to_string(fg_b) + "m";

    if(bg_r != 0 && bg_g != 0 && bg_b != 0)
        print += "\033[48;2;" + std::to_string(bg_r) + ";" + std::to_string(bg_g) + ";" + std::to_string(bg_b) + "m";
    print += text;
    print += "\033[0m";
    return print;
}

void TerminalWriter::write_image(ANSIImage &image, bool centered) {
    auto image_buffer = image.get_image(); 

    std::string finalImage = "";
    
    int x_offset = (m_width - image.get_width()) / 2;
    int y_offset = (m_height - image.get_height()) / 2 + 5;

    for (auto charData: image_buffer) {
        int centered_x = x_offset + charData.x + 1;
        int centered_y = y_offset + charData.y + 1;

        if (centered) {
            if(centered_x >= 1 && centered_x <= m_width && centered_y >= 1 && centered_y <= m_height) {
                finalImage += print_at_position(centered_x, centered_y, charData.utf_char, charData.fg_color, charData.bg_color);
            }
        } else {
            finalImage += print_at_position(charData.x, charData.y, charData.utf_char, charData.fg_color, charData.bg_color);
        }      
    }
    write_string(finalImage);
    m_screenBuffer += finalImage;
}
}
