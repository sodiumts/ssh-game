#pragma once

#include <memory>
#include <libssh/libssh.h>

#include "TerminalWriter.hpp"

namespace sshGame {
class GameHandler {
    public:
        GameHandler(ssh_channel &channel, int width, int height, const std::string &username);      
        ~GameHandler();
        
        void init();
        void quit();
    
        void receiveInput(const std::string &input);
        void receiveScreenChange(int width, int height);
        void displayIncreaseSize();
    private:
        void initialScreen();
        void drawBorder();
        void drawControls();

        std::unique_ptr<TerminalWriter> m_terminalWriter; 
        std::string m_username;

        ANSIImage m_nightImage{"../assets/night_view.csv"};
        
        int m_width;
        int m_height;
        
        std::string m_storedBuffer;
        bool m_firstTooSmall = true;
};
}
