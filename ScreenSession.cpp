#include "ScreenSession.h"
#include "Scheduler.h"
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

void clearScreenSession() {
    std::system("cls");
}

void screenSessionInterface(ScreenSession& session) {
    std::system("cls"); 

    std::cout << "\033[1;1H";  
    std::cout << "\n";
    std::cout << "\033[31m=========== SCREEN : " << session.name << " ===========\033[0m\n";
    
    // Check if this is a real process from the scheduler
    Process* realProcess = globalScheduler.getProcess(session.name);
    if (realProcess) {
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
            if (realProcess->coreId >= 0) {
                std::cout << "Core                  : " << realProcess->coreId << "\n";
            }
        }
    } else {
        // Fallback to session data for manually created screens
        std::cout << "Process Name          : " << session.name << "\n";
        std::cout << "Instruction Progress  : " << session.currentLine << " / " << session.totalLines << "\n";
        std::cout << "Created At            : " << session.timestamp << "\n";
        std::cout << "Status                : MANUAL SESSION\n";
    }
    
    std::cout << "\n\033[33mType 'exit' to return to main menu.\033[0m\n";
    std::cout << "\033[34mUse: 'process-smi' to print information about the process\033[0m\n\n";

    int baseLine = 10; 
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
            
            // Refresh process information
            Process* updatedProcess = globalScheduler.getProcess(session.name);
            if (updatedProcess) {
                std::cout << "Process Name          : " << updatedProcess->name << "\n";
                std::cout << "Instruction Progress  : " << updatedProcess->currentInstruction << " / " << updatedProcess->instructions.size() << "\n";
                
                auto time_t = std::chrono::system_clock::to_time_t(updatedProcess->creationTime);
                std::cout << "Created At            : " << std::put_time(std::localtime(&time_t), "%m/%d/%Y, %I:%M:%S %p") << "\n";
                
                if (updatedProcess->isFinished) {
                    std::cout << "Status                : FINISHED\n";
                } else {
                    std::string status = (updatedProcess->state == ProcessState::RUNNING) ? "RUNNING" : "READY";
                    std::cout << "Status                : " << status << "\n";
                    if (updatedProcess->coreId >= 0) {
                        std::cout << "Core                  : " << updatedProcess->coreId << "\n";
                    }
                }
            } else {
                std::cout << "Process Name          : " << session.name << "\n";
                std::cout << "Instruction Progress  : " << session.currentLine << " / " << session.totalLines << "\n";
                std::cout << "Created At            : " << session.timestamp << "\n";
                std::cout << "Status                : MANUAL SESSION\n";
            }
            
            std::cout << "\n\033[33mType 'exit' to return to main menu.\033[0m\n";
            std::cout << "\033[34mUse: 'process-smi' to print information about the process\033[0m\n\n";
            currentLine = baseLine;
        } else if (input == "process-smi") {
            // Show detailed process information
            Process* smiProcess = globalScheduler.getProcess(session.name);
            if (smiProcess) {
                std::cout << "\033[" << (currentLine + 1) << ";1H";
                std::cout << "=== Process SMI for " << smiProcess->name << " ===";
                std::cout << "\033[" << (currentLine + 2) << ";1H";
                std::cout << "Process ID: " << smiProcess->id;
                std::cout << "\033[" << (currentLine + 3) << ";1H";
                std::cout << "Current Instruction: " << smiProcess->currentInstruction << " / " << smiProcess->instructions.size();
                std::cout << "\033[" << (currentLine + 4) << ";1H";
                std::cout << "Memory Variables: " << smiProcess->variables.size();
                std::cout << "\033[" << (currentLine + 5) << ";1H";
                if (smiProcess->isFinished) {
                    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                        smiProcess->finishTime - smiProcess->creationTime).count();
                    std::cout << "Runtime: " << std::fixed << std::setprecision(1) << elapsed/1000.0 << "s (FINISHED)";
                } else {
                    auto currentTime = std::chrono::system_clock::now();
                    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                        currentTime - smiProcess->creationTime).count();
                    std::cout << "Runtime: " << std::fixed << std::setprecision(1) << elapsed/1000.0 << "s";
                }
                currentLine += 6;
            } else {
                std::cout << "\033[" << (currentLine + 1) << ";1H";
                std::cout << "No scheduler process found for " << session.name;
                currentLine += 2;
            }
        } else {
            // For manual sessions, simulate instruction progress
            if (!globalScheduler.getProcess(session.name)) {
                session.currentLine = std::min(session.totalLines, session.currentLine + 1);
                // Update only the instruction progress line (line 4)
                std::cout << "\033[4;1H\033[2K";
                std::cout << "Instruction Progress  : " << session.currentLine << " / " << session.totalLines;
                std::cout.flush();
            }

            // Print message below the prompt
            std::cout << "\033[" << (currentLine + 1) << ";1H";
            std::cout << "'" << input << "' command is not supported on the screen yet.";
            std::cout.flush();

            // Move prompt line down for next input
            currentLine += 2;
        }
    }

    clearScreenSession();
}

void handleScreenCommand(const std::string& command) {
    std::istringstream iss(command);
    std::string cmd, flag, name;
    iss >> cmd >> flag >> name;
    
    if (flag == "-s") {
        // Check if this process exists in the scheduler first
        Process* schedulerProcess = globalScheduler.getProcess(name);
        
        if (schedulerProcess) {
            // Create session for existing scheduler process
            if (screens.count(name)) {
                std::cout << "\nScreen session '" << name << "' already exists.\n";
            } else {
                ScreenSession newSession = {
                    name,
                    schedulerProcess->currentInstruction,
                    (int)schedulerProcess->instructions.size(),
                    getCurrentTimestamp()
                };
                screens[name] = newSession;
                screenSessionInterface(screens[name]);
            }
        } else if (screens.count(name)) {
            std::cout << "\nScreen session '" << name << "' already exists.\n";
        } else {
            // Create manual session
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
        if (!screens.count(name) && !globalScheduler.getProcess(name)) {
            std::cout << "\nNo session named '" << name << "' found.\n";
        } else {
            if (!screens.count(name)) {
                // Create session for scheduler process
                Process* schedulerProcess = globalScheduler.getProcess(name);
                ScreenSession newSession = {
                    name,
                    schedulerProcess->currentInstruction,
                    (int)schedulerProcess->instructions.size(),
                    getCurrentTimestamp()
                };
                screens[name] = newSession;
            }
            screenSessionInterface(screens[name]);
        }
    } else if (flag == "-ls") {
        // Show process list from scheduler
        std::cout << "\n";
        globalScheduler.printScreen();
    } else {
        std::cout << "\nInvalid screen usage. Try: screen -s <name>, screen -r <name>, or screen -ls\n";
    }
}