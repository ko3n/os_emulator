#ifndef SCREEN_SESSION_H
#define SCREEN_SESSION_H

#include <string>
#include <map>

struct ScreenSession {
    std::string name;
    int currentLine;
    int totalLines;
    std::string timestamp;
};

// Function declarations
std::string getCurrentTimestamp();
void screenSessionInterface(ScreenSession& session);
void handleScreenCommand(const std::string& command);
void clearScreen(); 

// Global variable declaration
extern std::map<std::string, ScreenSession> screens;

#endif 