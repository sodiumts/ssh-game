#include "GameHandler.hpp"

#include <memory>
#include <print>

namespace sshGame {

GameHandler::GameHandler(ssh_channel &channel, int width, int height, const std::string &username) : m_username(username), m_width(width), m_height(height) {
    m_terminalWriter = std::make_unique<TerminalWriter>(channel, width, height);
}

GameHandler::~GameHandler() {
}

void GameHandler::init() {
    initialScreen();
    if (m_width < MIN_WIDTH || m_height < MIN_HEIGHT) {
        receiveScreenChange(m_width, m_height);
    }
}

void GameHandler::quit() {
    m_terminalWriter->clear_screen();
    m_terminalWriter->enable_cursor();
    m_terminalWriter->alternate_screen_buffer_disable();
}

void GameHandler::receiveInput(const std::string &input) {
    if(input.length() > 1)
        return;
    
    std::println("User {}: {}", m_username, input);
}

void GameHandler::receiveScreenChange(int width, int height) {
    m_width = width;
    m_height = height;
    
    m_terminalWriter->update_terminal(m_width, m_height);
    if (m_width < MIN_WIDTH || m_height < MIN_HEIGHT) {
        if (m_firstTooSmall) {
            m_storedBuffer = m_terminalWriter->get_buffer();
            m_firstTooSmall = false;
        }
        
        m_terminalWriter->clear_buffer();
        displayIncreaseSize();
        return;
    } else if (!m_firstTooSmall) {
        m_terminalWriter->clear_screen();
        m_terminalWriter->clear_buffer();
        m_terminalWriter->write_buffer(m_storedBuffer);
        m_firstTooSmall = true; 
    }
    m_terminalWriter->clear_screen();
    //drawControls();
    m_terminalWriter->write_image("../assets/night_view.csv");
}

void GameHandler::displayIncreaseSize() {
    m_terminalWriter->clear_screen();
    std::string centered = m_terminalWriter->get_centered_text("<< Increase the size of the terminal >>");
    m_terminalWriter->write_string(centered);
}

void GameHandler::drawBorder() {

}   

void GameHandler::drawControls() {
    std::wstring controls; 
    
    std::string bar = "";
    int barsize = 50;

    for (int i = 0; i < barsize; i++) {
        bar += "─";
    }

    std::string barColor = "#5D4F75";
    int bottomPadding = m_height / 8;
    
    int left_padding = (m_width - barsize) / 2;
    std::string formattedBar = m_terminalWriter->print_text(bar, left_padding, m_height - bottomPadding, barColor);
    m_terminalWriter->clear_screen(); 

    m_terminalWriter->write_string(formattedBar);
    
    std::string controlsBar = m_terminalWriter->print_text("↑↓", left_padding + 9, m_height - bottomPadding + 1);
    controlsBar += m_terminalWriter->print_text("select", left_padding + 12, m_height - bottomPadding + 1, "#848684");

    controlsBar += m_terminalWriter->print_text("q", left_padding + 31, m_height - bottomPadding + 1);
    controlsBar += m_terminalWriter->print_text("quit", left_padding + 33, m_height - bottomPadding + 1, "#848684");
    m_terminalWriter->write_string(controlsBar);
}

void GameHandler::initialScreen() {
    m_terminalWriter->clear_screen();
    m_terminalWriter->disable_cursor();
    m_terminalWriter->alternate_screen_buffer_enable();
    m_terminalWriter->write_image("../assets/night_view.csv");
    //drawBorder();
    //drawControls();
    

}

}
