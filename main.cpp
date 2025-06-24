#include <iostream>
#include <string>
#include <ctime>
#include <iomanip>
#include <sstream>
#include "ScreenSession.h"
#include "Config.h"

// Global variables
bool isInitialized = false;

void printHeader() {
    std::cout << R"(
      OO O o o o...      ______________________ _________________
  O     ____          |                    | |               |
 ][_n_i_| (   ooo___  |                    | |               |
(__________|_[______]_|____________________|_|_______________|
  0--0--0      0  0      0       0     0        0        0      Choo-Choo OS Emulator
)" << "\n";
    std::cout << "\033[32mHello, Welcome to CSOPESY commandline!\033[0m\n";
    std::cout << "\033[33mType 'exit' to quit, 'clear' to clear the screen\033[0m\n\n";
    std::cout << "\033[34mUse: 'initialize' to initialize the processor configurtation\033[0m\n";
    std::cout << "\033[34m     'screen -s <name>' to start a screen\033[0m\n";
    std::cout << "\033[34m     'screen -r <name>' to resume a screen\033[0m\n";
    std::cout << "\033[34m     'scheduler-start' to generate dummy processess which is accessible via the 'screen' command\033[0m\n";
    std::cout << "\033[34m     'scheduler-stop' to stop generating dummy processess which is accessible via the 'screen' command\033[0m\n";
    std::cout << "\033[34m     'report-util' to generate CPU utilization report\033[0m\n";
}

void clearScreen() {
    std::system("cls");
    printHeader();
}

void initialize() {
    if (loadConfig("config.txt")) {
        std::cout << "Configuration loaded successfully:\n\n";
        std::cout << "- numCPU: " << systemConfig.numCPU << "\n";
        std::cout << "- scheduler: " << systemConfig.scheduler << "\n";
        std::cout << "- quantumCycles: " << systemConfig.quantumCycles << "\n";
        std::cout << "- batchProcessFreq: " << systemConfig.batchProcessFreq << "\n";
        std::cout << "- minInstructions: " << systemConfig.minInstructions << "\n";
        std::cout << "- maxInstructions: " << systemConfig.maxInstructions << "\n";
        std::cout << "- delayPerExec: " << systemConfig.delayPerExec << "\n";
        isInitialized = true;
    } else {
        std::cout << "Failed to load configuration.\n";
    }
}

void schedulerTest() {
    std::cout << "scheduler-test command recognized. Doing something...\n";
}

void schedulerStop() {
    std::cout << "scheduler-stop command recognized. Doing something...\n";
}

void reportUtil() {
    std::cout << "report-util command recognized. Doing something...\n";
}

int main() {
    std::string userInput;
    printHeader();

    while (true) {
        std::cout << "\n>";
        std::getline(std::cin, userInput);

        if (userInput == "initialize") {
            std::cout << "\n";
            initialize();
        }
        else if (userInput == "clear") {
            clearScreen();
        }
        else if (userInput == "exit") {
            break;
        }
        else if (!isInitialized) {
            std::cout << "\n";
            std::cout << "Please run 'initialize' command first.\n"; // Except for clear and exit
        }
        else if (userInput == "scheduler-test") {
            std::cout << "\n";
            schedulerTest();
        }
        else if (userInput == "scheduler-stop") {
            std::cout << "\n";
            schedulerStop();
        }
        else if (userInput == "report-util") {
            std::cout << "\n";
            reportUtil();
        }
        else if (userInput.rfind("screen", 0) == 0) {
            handleScreenCommand(userInput);
        }
        else {
            std::cout << "\n";
            std::cout << "Unknown command. Please try again\n";
        }
    }

    return 0;
}