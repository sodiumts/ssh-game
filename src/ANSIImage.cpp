
#include "ANSIImage.hpp"
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace sshGame {
ANSIImage::ANSIImage(const std::string &csv_path) {
    loadImage(csv_path);
}

void ANSIImage::loadImage(const std::string &image_path) {
    std::ifstream file(image_path);
    if (!file.is_open()) {
        throw std::runtime_error("failed to open file");
    }
    std::string image = "";
    std::string line;

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

        m_imageBuffer.push_back({x,y,extended_ascii_to_utf8(ascii_code),fg_color,bg_color});
        
        min_x = std::min(min_x, x);
        max_x = std::max(max_x, x);
        min_y = std::min(min_y, y);
        max_y = std::max(max_y, y);
    }

    m_width = max_x - min_x + 1;
    m_height = max_y - min_y + 1;

}

std::string ANSIImage::unicode_to_utf8(int codepoint) {
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

std::string ANSIImage::extended_ascii_to_utf8(int ascii_code) {
    wchar_t mapping = mappings[ascii_code]; 
    return unicode_to_utf8(mapping);
}


}
