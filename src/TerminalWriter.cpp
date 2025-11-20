#include "TerminalWriter.hpp"

#include <fstream>
#include <vector>
#include <sstream>
#include <print>

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

std::vector<int> mappings = {0, 9786, 9787, 9829, 9830, 9827, 9824, 8226, 9688, 9675, 9689, 9794, 9792, 9834, 9835, 9788, 9658, 9668, 8597, 8252, 182, 167, 9644, 8616, 8593, 8595, 8594, 8592, 8735, 8596, 9650, 9660, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 8962, 199, 252, 233, 226, 228, 224, 229, 231, 234, 235, 232, 239, 238, 236, 196, 197, 201, 230, 198, 244, 246, 242, 251, 249, 255, 214, 220, 162, 163, 165, 8359, 402, 225, 237, 243, 250, 241, 209, 170, 186, 191, 8976, 172, 189, 188, 161, 171, 187, 9617, 9618, 9619, 9474, 9508, 9569, 9570, 9558, 9557, 9571, 9553, 9559, 9565, 9564, 9563, 9488, 9492, 9524, 9516, 9500, 9472, 9532, 9566, 9567, 9562, 9556, 9577, 9574, 9568, 9552, 9580, 9575, 9576, 9572, 9573, 9561, 9560, 9554, 9555, 9579, 9578, 9496, 9484, 9608, 9604, 9612, 9616, 9600, 945, 223, 915, 960, 931, 963, 181, 964, 934, 920, 937, 948, 8734, 966, 949, 8745, 8801, 177, 8805, 8804, 8992, 8993, 247, 8776, 176, 8729, 183, 8730, 8319, 178, 9632, 160};
std::string unicode_to_utf8(int codepoint) {
    std::string utf8_string;
    
    if (codepoint < 0x80) {
        utf8_string.push_back(static_cast<char>(codepoint));
    } else if (codepoint < 0x800) {
        utf8_string.push_back(static_cast<char>((codepoint >> 6) | 0xC0));
        utf8_string.push_back(static_cast<char>((codepoint & 0x3F) | 0x80));
    } else if (codepoint < 0x10000) {
        utf8_string.push_back(static_cast<char>((codepoint >> 12) | 0xE0));
        utf8_string.push_back(static_cast<char>(((codepoint >> 6) & 0x3F) | 0x80));
        utf8_string.push_back(static_cast<char>((codepoint & 0x3F) | 0x80));
    } else if (codepoint < 0x110000) {
        utf8_string.push_back(static_cast<char>((codepoint >> 18) | 0xF0));
        utf8_string.push_back(static_cast<char>(((codepoint >> 12) & 0x3F) | 0x80));
        utf8_string.push_back(static_cast<char>(((codepoint >> 6) & 0x3F) | 0x80));
        utf8_string.push_back(static_cast<char>((codepoint & 0x3F) | 0x80));
    } else {
        throw std::invalid_argument("Invalid Unicode codepoint");
    }
    
    return utf8_string;
}
std::string extended_ascii_to_utf8(int ascii_code) {
    wchar_t mapping = mappings[ascii_code]; 
    return unicode_to_utf8(mapping);
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

void TerminalWriter::write_image(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("failed to open file");
    }
    std::string image = "";
    std::string line;
    
    
    std::vector<std::tuple<int, int, int, std::string, std::string>> image_data;
    int min_x = 20000, max_x = 0, min_y = 20000, max_y = 0;

    bool first = true;

    while(std::getline(file, line)) {
        if(first) {
            first = false;
            continue;
        }
        std::stringstream ss(line);

        int x, y, ascii_code;
        std::string fg_color, bg_color;
        std::string temp;

        std::getline(ss, temp, ',');
        x = std::stoi(temp);

        std::getline(ss, temp, ',');
        y = std::stoi(temp);

        std::getline(ss, temp, ','); 
        ascii_code = std::stoi(temp);

        std::getline(ss, fg_color, ','); 
        std::getline(ss, bg_color, ',');
        //image += print_at_position(x, y, extended_ascii_to_utf8(ascii_code), fg_color, bg_color);
	image_data.push_back({x,y,ascii_code,fg_color,bg_color});
	min_x = std::min(min_x, x);
        max_x = std::max(max_x, x);
        min_y = std::min(min_y, y);
        max_y = std::max(max_y, y);
    }

     int image_width = max_x - min_x + 1;
    int image_height = max_y - min_y + 1;
    
    int x_offset = (m_width - image_width) / 2 - min_x + 1;
    int y_offset = (m_height - image_height) / 2 - min_y + 1 + 5;
    //std::println("Image bounds: x[{}, {}] y[{}, {}]", min_x, max_x, min_y, max_y);
    //std::println("Image dimensions: {}x{}", image_width, image_height);
    //std::println("Terminal dimensions: {}x{}", m_width, m_height);
    //std::println("Offsets: x={}, y={}", x_offset, y_offset);
    // Second pass: build the image with centered coordinates
    for(const auto& [x, y, ascii_code, fg_color, bg_color] : image_data) {
        int centered_x = x_offset + (x - min_x) + 1;  // +1 because terminal is 1-indexed
        int centered_y = y_offset + (y - min_y) + 1;  // +1 because terminal is 1-indexed
        
        // Ensure coordinates are within terminal bounds
        if(centered_x >= 1 && centered_x <= m_width && centered_y >= 1 && centered_y <= m_height) {
            image += print_at_position(centered_x, centered_y, extended_ascii_to_utf8(ascii_code), fg_color, bg_color);
        }
    }

    write_string(image);
    m_screenBuffer += image;
}
}
