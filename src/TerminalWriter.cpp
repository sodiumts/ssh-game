#include "TerminalWriter.hpp"

namespace sshGame {

TerminalWriter::TerminalWriter(ssh_channel &channel) : m_channel(channel) {}
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

void TerminalWriter::enable_cursor() {
    write_string("\e[?25h");
}

void TerminalWriter::disable_cursor() {
    write_string("\e[?25l");
}
}
