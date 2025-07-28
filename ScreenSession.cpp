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
    iss >> cmd >> flag >> name;
    
    if (flag == "-s") {
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

        // If process does not exist yet, create it and attach to a screen session
        if (globalScheduler) {
            globalScheduler->addProcess(name);
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
            screenSessionInterface(screens[name]);
        } else {
            std::cout << "\nFailed to create process in scheduler.\n";
        }
    } else if (flag == "-r") {
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
    } else if (flag == "-ls") {
        // Show process list from scheduler
        std::cout << "\n";
        if (globalScheduler) globalScheduler->printScreen();
    } else {
        std::cout << "\nInvalid screen usage. Try: screen -s <name>, screen -r <name>, or screen -ls\n";
    }
}