#include "ScreenSession.h"
#include "Scheduler.h"
extern Scheduler* globalScheduler;
#include <iostream>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <algorithm>

// Forward declaration of clearScreen from main.cpp
extern void clearScreen();

std::map<std::string, ScreenSession> screens;

std::string getCurrentTimestamp() {
    std::time_t now = std::time(nullptr);
    std::tm* local = std::localtime(&now);

    std::ostringstream oss;
    oss << std::put_time(local, "%m/%d/%Y, %I:%M:%S %p");
    return oss.str();
}

void displayHeader(const std::string& sessionName) {
    Process* realProcess = globalScheduler ? globalScheduler->getProcess(sessionName) : nullptr;
    if (!realProcess) return;
    
    std::cout << "\033[1;1H";  
    std::cout << "\n";
    std::cout << "\033[31m=========== SCREEN : " << sessionName << " ===========\033[0m\n";
    std::cout << "Process Name          : " << realProcess->name << "\n";
    std::cout << "Instruction Progress  : " << realProcess->currentInstruction << " / " << realProcess->instructions.size() << "\n";
    
    // Format creation time
    auto time_t = std::chrono::system_clock::to_time_t(realProcess->creationTime);
    std::cout << "Created At            : " << std::put_time(std::localtime(&time_t), "%m/%d/%Y, %I:%M:%S %p") << "\n";
    
    // Show status
    if (realProcess->isFinished) {
        std::cout << "Status                : FINISHED\n";
    } else {
        std::string status = (realProcess->state == ProcessState::RUNNING) ? "RUNNING" : "READY";
        std::cout << "Status                : " << status << "\n";
    }
    
    // Show core
    if (realProcess->coreId >= 0) {
        std::cout << "Core                  : " << realProcess->coreId << "\n";
    } else {
        std::cout << "Core                  : Not assigned\n";
    }
}

void screenSessionInterface(ScreenSession& session) {
    std::system("cls"); 
    displayHeader(session.name);

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
            displayHeader(session.name);

            currentLine = baseLine;
            
        } else if (input == "process-smi") {
            // Show detailed process information
            Process* smiProcess = globalScheduler ? globalScheduler->getProcess(session.name) : nullptr;
            if (smiProcess) {
                int printedLines = 0;

                currentLine += 2;

                auto printLine = [&](const std::string& text) {
                    std::cout << "\033[" << (currentLine + printedLines) << ";1H\033[2K";
                    std::cout << text << std::endl;
                    ++printedLines;
                };

                printLine("Process name: " + smiProcess->name);
                printLine("ID: " + std::to_string(smiProcess->id));
                printLine("Logs:");
                
            
                for (int i = 0; i < smiProcess->currentInstruction && i < smiProcess->instructions.size(); ++i) {
                    const Instruction& instr = smiProcess->instructions[i];
                    if (instr.type == InstructionType::PRINT) {

                        /*std::time_t now = std::time(nullptr);
                        std::tm* local = std::localtime(&now);
                        std::ostringstream timestamp;
                        timestamp << "(" << std::put_time(local, "%m/%d/%Y %I:%M:%S%p") << ")";
                        printLine(timestamp.str() + " Core:" + std::to_string(smiProcess->coreId) + " " + instr.msg);*/

                        std::ostringstream timestamp;
                        
                        if (instr.executedAt.has_value()) {
                            std::time_t execTime = std::chrono::system_clock::to_time_t(instr.executedAt.value());
                            std::tm* local = std::localtime(&execTime);
                            timestamp << "(" << std::put_time(local, "%m/%d/%Y %I:%M:%S %p") << ")";
                        } else {
                            timestamp << "(Time N/A)";
                        }

                        printLine(timestamp.str() + " Core:" + std::to_string(smiProcess->coreId) + " " + instr.msg);
                    }
                }
                
                printLine("");

                //current instruction line & total lines of code
                printLine("Current instruction line: " + std::to_string(smiProcess->currentInstruction));
                printLine("Lines of code: " + std::to_string(smiProcess->instructions.size()));

                if (smiProcess->isFinished) {
                    printLine("Finished!");
                }
                
                printLine("");
                currentLine += printedLines + 2;

            } else {
                std::cout << "\033[" << (currentLine + 1) << ";1H\033[2K";
                std::cout << "No scheduler process found for " << session.name << std::endl;
                currentLine += 3;
            }

        } else {
            // Print message below the prompt
            currentLine += 1;
            std::cout << "\033[" << currentLine << ";1H\033[2K";
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
    iss >> cmd >> flag;
    
    if (flag == "-s") {
        std::string memoryStr;
        iss >> name >> memoryStr;
        
        // Check if both name and memory size are provided
        if (name.empty() || memoryStr.empty()) {
            std::cout << "\nUsage: screen -s <process_name> <memory_size>\n";
            std::cout << "Memory size must be power of 2 between 64 and 65536 bytes.\n";
            return;
        }
        
        int memorySize;
        try {
            memorySize = std::stoi(memoryStr);
        } catch (const std::exception& e) {
            std::cout << "\nInvalid memory size format. Please enter a valid number.\n";
            return;
        }
        
        // Validate memory size: must be power of 2 and within [64, 65536] range
        auto isPowerOf2 = [](int n) { return n > 0 && (n & (n - 1)) == 0; };
        
        if (!isPowerOf2(memorySize) || memorySize < 64 || memorySize > 65536) {
            std::cout << "\nInvalid memory allocation. Memory must be power of 2 between 64 and 65536 bytes.\n";
            return;
        }
        
        // Check if the process already exists in the scheduler
        if (globalScheduler && globalScheduler->getProcess(name)) {
            std::cout << "\nProcess '" << name << "' already exists. Cannot use 'screen -s' on existing processes.\n";
            return;
        }

        // Check if a screen session already exists
        if (screens.count(name)) {
            std::cout << "\nScreen session '" << name << "' already exists.\n";
            return;
        }

        // Create process with specified memory size
        if (globalScheduler) {
            globalScheduler->addProcessWithMemory(name, memorySize);
        }
        
        Process* newProcess = globalScheduler ? globalScheduler->getProcess(name) : nullptr;

        if (newProcess) {
            ScreenSession newSession = {
                name,
                newProcess->currentInstruction,
                (int)newProcess->instructions.size(),
                getCurrentTimestamp()
            };
            screens[name] = newSession;
            std::cout << "\nProcess '" << name << "' created with " << memorySize << " bytes of memory.\n";
            screenSessionInterface(screens[name]);
        } else {
            std::cout << "\nFailed to create process in scheduler - insufficient memory.\n";
        }
    } 
    else if (flag == "-r") {
        iss >> name; // Read the name for -r flag
        
        if (!screens.count(name) && (!globalScheduler || !globalScheduler->getProcess(name))) {
            std::cout << "\nNo session named '" << name << "' found.\n";
        } else {
            if (!screens.count(name)) {
                // Create session for scheduler process
                Process* schedulerProcess = globalScheduler ? globalScheduler->getProcess(name) : nullptr;
                if (schedulerProcess) {
                    ScreenSession newSession = {
                        name,
                        schedulerProcess->currentInstruction,
                        (int)schedulerProcess->instructions.size(),
                        getCurrentTimestamp()
                    };
                    screens[name] = newSession;
                }
            }
            screenSessionInterface(screens[name]);
        }
    } 
    else if (flag == "-ls") {
        // Show process list from scheduler
        std::cout << "\n";
        if (globalScheduler) globalScheduler->printScreen();
    } 
    else {
        std::cout << "\nInvalid screen usage.\n";
    }
}