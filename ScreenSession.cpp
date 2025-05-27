#include "ScreenSession.h"
#include <iostream>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <algorithm>

std::map<std::string, ScreenSession> screens;

std::string getCurrentTimestamp() {
    std::time_t now = std::time(nullptr);
    std::tm* local = std::localtime(&now);

    std::ostringstream oss;
    oss << std::put_time(local, "%m/%d/%Y, %I:%M:%S %p");
    return oss.str();
}

void screenSessionInterface(ScreenSession& session) {
    std::system("cls"); 

    std::cout << "\033[1;1H";  
    std::cout << "\n";
    std::cout << "\033[31m=========== SCREEN : " << session.name << " ===========\033[0m\n";
    std::cout << "Process Name          : " << session.name << "\n";
    std::cout << "Instruction Progress  : " << session.currentLine << " / " << session.totalLines << "\n";
    std::cout << "Created At            : " << session.timestamp << "\n";
    std::cout << "\n\033[33mType 'exit' to return to main menu.\033[0m\n";

    int baseLine = 9; 
    int currentLine = baseLine;

    std::string input;
    while (true) {
        // Move to the current prompt line
        std::cout << "\033[" << currentLine << ";1H";
        std::cout << "(" << session.name << ")> ";
        std::cout.flush();

        std::getline(std::cin, input);

        if (input == "exit") {
            break;
        } else if (input == "clear") {
            std::system("cls");
            std::cout << "\033[1;1H";  
            std::cout << "\n";
            std::cout << "\033[31m=========== SCREEN : " << session.name << " ===========\033[0m\n";
            std::cout << "Process Name          : " << session.name << "\n";
            std::cout << "Instruction Progress  : " << session.currentLine << " / " << session.totalLines << "\n";
            std::cout << "Created At            : " << session.timestamp << "\n";
            std::cout << "\n\033[33mType 'exit' to return to main menu.\033[0m\n";
            currentLine = baseLine;
        } else {
            session.currentLine = std::min(session.totalLines, session.currentLine + 1);
            // Update only the instruction progress line (line 4)
            std::cout << "\033[4;1H\033[2K";
            std::cout << "Instruction Progress  : " << session.currentLine << " / " << session.totalLines;
            std::cout.flush();

            // Print message below the prompt
            std::cout << "\033[" << (currentLine + 1) << ";1H";
            std::cout << "'" << input << "' command is not supported on the screen yet.";
            std::cout.flush();

            // Move prompt line down for next input
            currentLine += 2;
        }
    }

    clearScreen();
}

void handleScreenCommand(const std::string& command) {
    std::istringstream iss(command);
    std::string cmd, flag, name;
    iss >> cmd >> flag >> name;
    
    if (flag == "-s") {
        if (screens.count(name)) {
            std::cout << "Screen session '" << name << "' already exists.\n";
        } else {
            ScreenSession newSession = {
                name,
                1, // current line
                100, // total lines
                getCurrentTimestamp()
            };
            screens[name] = newSession;
            screenSessionInterface(screens[name]);
        }
    } else if (flag == "-r") {
        if (!screens.count(name)) {
            std::cout << "No session named '" << name << "' found.\n";
        } else {
            screenSessionInterface(screens[name]);
        }
    } else {
        std::cout << "Invalid screen usage. Try: screen -s <name> or screen -r <name>\n";
    }
}

