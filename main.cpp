#include <iostream>
#include <string>
#include <ctime>
#include <iomanip>
#include <sstream>
#include "ScreenSession.h"
#include "Config.h"
#include "Scheduler.h"


// Global variables
bool isInitialized = false;
Scheduler* globalScheduler = nullptr;

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
    std::cout << "\033[34mUse: 'initialize' to initialize the processor configuration\033[0m\n";
    std::cout << "\033[34m     'screen -s <name> <memory size> <instructions>' to start a screen\033[0m\n";
    std::cout << "\033[34m     'screen -c <name> <memory size>' to create a process with a custom set of instructions\033[0m\n";
    std::cout << "\033[34m     'screen -r <name>' to resume a screen\033[0m\n";
    std::cout << "\033[34m     'screen -ls' to list all processes\033[0m\n";
    std::cout << "\033[34m     'scheduler-test' to start the scheduler\033[0m\n";
    std::cout << "\033[34m     'scheduler-stop' to stop the scheduler\033[0m\n";
    std::cout << "\033[34m     'report-util' to generate CPU utilization report\033[0m\n";
    std::cout << "\033[34m     'process-smi' to provide a high-level overview of available/used memory\033[0m\n";
    std::cout << "\033[34m     'vmstat' to provide fine-grained memory details\033[0m\n";
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
        std::cout << "- maxOverallMem: " << systemConfig.maxOverallMem << "\n";
        std::cout << "- memPerFrame: " << systemConfig.memPerFrame << "\n";
        std::cout << "- minMemPerProc: " << systemConfig.minMemPerProc << "\n";
        std::cout << "- maxMemPerProc: " << systemConfig.maxMemPerProc << "\n\n";
        
        // Initialize the scheduler
        if (globalScheduler) delete globalScheduler;
        globalScheduler = new Scheduler();
        if (globalScheduler->initialize()) {
            isInitialized = true;
        } else {
            std::cout << "Failed to initialize scheduler.\n";
        }
    } else {
        std::cout << "Failed to load configuration.\n";
    }
}

void schedulerTest() {
    if (!isInitialized) {
        std::cout << "Please run 'initialize' command first.\n";
        return;
    }
    globalScheduler->schedulerTest();
}

void schedulerStop() {
    if (!isInitialized) {
        std::cout << "Please run 'initialize' command first.\n";
        return;
    }
    globalScheduler->schedulerStop();
}

void reportUtil() {
    if (!isInitialized) {
        std::cout << "Please run 'initialize' command first.\n";
        return;
    }
    globalScheduler->reportUtil();
}

void vmstat() {
    if (!isInitialized) {
        std::cout << "Please run 'initialize' command first.\n";
        return;
    }
    
    auto& memManager = globalScheduler->getMemoryManager();
    auto& scheduler = *globalScheduler;
    
    // Memory information (in bytes as specified)
    int totalMemory = systemConfig.maxOverallMem;
    int usedMemory = memManager.getUsedFrames() * systemConfig.memPerFrame;
    int freeMemory = totalMemory - usedMemory;
    
    std::cout << std::setw(12) << totalMemory << " K total memory\n";
    std::cout << std::setw(12) << usedMemory << " K used memory\n";
    std::cout << std::setw(12) << freeMemory << " K free memory\n";
    std::cout << std::setw(12) << scheduler.getIdleCpuTicks() << " idle cpu ticks\n";
    std::cout << std::setw(12) << scheduler.getActiveCpuTicks() << " active cpu ticks\n";
    std::cout << std::setw(12) << scheduler.getTotalCpuTicks() << " total cpu ticks\n";
    std::cout << std::setw(12) << scheduler.getNumPagedIn() << " num paged in\n";
    std::cout << std::setw(12) << scheduler.getNumPagedOut() << " num paged out\n";
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
            if (isInitialized && globalScheduler) {
                globalScheduler->schedulerStop();
                delete globalScheduler;
                globalScheduler = nullptr;
            }
            break;
        }
        else if (!isInitialized) {
            std::cout << "\n";
            std::cout << "Please run 'initialize' command first.\n";
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
        else if (userInput == "vmstat") {
            std::cout << "\n";
            vmstat();
        }
        else if (userInput == "process-smi") {
            std::cout << "\n";
            globalScheduler->processSmi();
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