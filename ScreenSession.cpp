#include "ScreenSession.h"
#include "Scheduler.h"
extern Scheduler* globalScheduler;
#include <iostream>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <regex>
#include "Config.h"

// Forward declaration of clearScreen from main.cpp
extern void clearScreen();
std::vector<Instruction> parseUserInstructions(const std::string& instrStr, std::string& errorMsg);
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

    // Track last printed instruction index (for any log message)
    int lastPrintedLogIdx = -1;

    while (true) {
        // Live log output: check for new instructions with non-empty msg executed since lastPrintedLogIdx
        Process* liveProcess = globalScheduler ? globalScheduler->getProcess(session.name) : nullptr;
        if (liveProcess) {
            int printedThisLoop = 0;
            for (int i = lastPrintedLogIdx + 1; i < liveProcess->currentInstruction && i < liveProcess->instructions.size(); ++i) {
                const Instruction& instr = liveProcess->instructions[i];
                if (!instr.msg.empty()) {
                    std::ostringstream timestamp;
                    if (instr.executedAt.has_value()) {
                        std::time_t execTime = std::chrono::system_clock::to_time_t(instr.executedAt.value());
                        std::tm* local = std::localtime(&execTime);
                        timestamp << "(" << std::put_time(local, "%m/%d/%Y %I:%M:%S %p") << ")";
                    } else {
                        timestamp << "(Time N/A)";
                    }
                    // Print at the next line
                    std::cout << "\033[" << (currentLine + printedThisLoop) << ";1H\033[2K";
                    std::cout << timestamp.str() << " Core:" << liveProcess->coreId << " " << instr.msg << std::endl;
                    lastPrintedLogIdx = i;
                    ++printedThisLoop;
                }
            }
            if (printedThisLoop > 0) {
                currentLine += printedThisLoop;
            }
        }

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
            lastPrintedLogIdx = -1;
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
                    std::ostringstream timestamp;
                    if (instr.executedAt.has_value()) {
                        std::time_t execTime = std::chrono::system_clock::to_time_t(instr.executedAt.value());
                        std::tm* local = std::localtime(&execTime);
                        timestamp << "(" << std::put_time(local, "%m/%d/%Y %I:%M:%S %p") << ")";
                    } else {
                        timestamp << "(Time N/A)";
                    }
                    if (!instr.msg.empty()) {
                        // Only print instructions with a message (DECLARE, PRINT, etc.), except ADD (handled below if no msg)
                        if (instr.type != InstructionType::ADD) {
                            printLine(timestamp.str() + " Core:" + std::to_string(smiProcess->coreId) + " " + instr.msg);
                        } else if (instr.type == InstructionType::ADD && instr.msg != "") {
                            // If ADD has a msg (e.g., custom), print it
                            printLine(timestamp.str() + " Core:" + std::to_string(smiProcess->coreId) + " " + instr.msg);
                        }
                    } else if (instr.type == InstructionType::ADD) {
                        // Show ADD log: e.g., ADD: 10 + 5 = 15
                        int left = 0, right = 0;
                        if (smiProcess->variables.count(instr.srcVar)) left = smiProcess->variables.at(instr.srcVar);
                        if (smiProcess->variables.count(instr.destVar)) right = smiProcess->variables.at(instr.destVar);
                        int result = left + right;
                        printLine(timestamp.str() + " Core:" + std::to_string(smiProcess->coreId) + " ADD: " + std::to_string(left) + " + " + std::to_string(right) + " = " + std::to_string(result));
                    }
                }
                printLine("");
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
    // Generate random instructions for the process
    std::vector<Instruction> randomInstructions;
    Process tempProcess(name, 0);
    tempProcess.generateRandomInstructions(systemConfig.minInstructions, systemConfig.maxInstructions);
    randomInstructions = tempProcess.instructions;
    globalScheduler->addProcessWithMemory(name, memorySize, randomInstructions);
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
    else if (flag == "-c") {
        iss >> name;
        std::string memStr, instrStr;
        int memorySize = 65536; // default to max

        // Peek next token to see if it's a number (memory size) or start of instructions
        std::streampos pos = iss.tellg();
        std::string nextToken;
        iss >> nextToken;
        bool hasMem = false;
        if (!nextToken.empty()) {
            try {
                size_t idx;
                int val = std::stoi(nextToken, &idx);
                if (idx == nextToken.size()) {
                    memorySize = val;
                    hasMem = true;
                } else {
                    iss.seekg(pos); // not a pure number, rewind
                }
            } catch (...) {
                iss.seekg(pos); // not a number, rewind
            }
        }

        // Now get the rest as instruction string
        std::getline(iss, instrStr);
        instrStr = instrStr.substr(instrStr.find_first_not_of(" \""));
        instrStr = instrStr.substr(0, instrStr.find_last_not_of("\"") + 1);

        if (name.empty() || instrStr.empty()) {
            std::cout << "\nUsage: screen -c <process_name> <memory_size> \"<instructions>\"\n";
            return;
        }

        auto isPowerOf2 = [](int n) { return n > 0 && (n & (n - 1)) == 0; };
        if (!isPowerOf2(memorySize) || memorySize < 64 || memorySize > 65536) {
            std::cout << "\nInvalid memory allocation. Memory must be power of 2 between 64 and 65536 bytes.\n";
            return;
        }
        // Parse instructions
        std::string errorMsg;
        auto instrs = parseUserInstructions(instrStr, errorMsg);
        if (!errorMsg.empty()) {
            std::cout << "\nInvalid command: " << errorMsg << "\n";
            return;
        }
        if (instrs.size() < 1 || instrs.size() > 50) {
            std::cout << "\nInvalid command: instruction count must be 1-50.\n";
            return;
        }
        // Create process with user instructions
        if (globalScheduler) {
            globalScheduler->addProcessWithMemory(name, memorySize, instrs);
        }
        Process* newProcess = globalScheduler ? globalScheduler->getProcess(name) : nullptr;
        if (newProcess) {
            ScreenSession newSession = { name, newProcess->currentInstruction, (int)newProcess->instructions.size(), getCurrentTimestamp() };
            screens[name] = newSession;
            std::cout << "\nProcess '" << name << "' created with " << memorySize << " bytes of memory.\n";
            screenSessionInterface(screens[name]);
        } else {
            std::cout << "\nFailed to create process in scheduler - insufficient memory.\n";
        }
    } 
    else {
        std::cout << "\nInvalid screen usage.\n";
    }
}

std::vector<Instruction> parseUserInstructions(const std::string& instrStr, std::string& errorMsg) {
    std::vector<Instruction> result;
    std::istringstream ss(instrStr);
    std::string token;
    int varCount = 0;
    std::set<std::string> declaredVars;

    while (std::getline(ss, token, ';')) {
        std::istringstream line(token);
        std::string cmd;
        line >> cmd;
        if (cmd == "DECLARE") {
            std::string var; int val;
            line >> var >> val;
            if (var.empty() || !line) { errorMsg = "Invalid DECLARE syntax"; return {}; }
            if (declaredVars.size() >= 32) continue; // ignore if symbol table full
            declaredVars.insert(var);
            Instruction instr(InstructionType::DECLARE, "", var, std::clamp(val, 0, 65535));
            result.push_back(instr);
        } else if (cmd == "ADD") {
            std::string dest, src1, src2;
            line >> dest >> src1 >> src2;
            if (dest.empty() || src1.empty() || src2.empty()) { errorMsg = "Invalid ADD syntax"; return {}; }
            Instruction instr(InstructionType::ADD, "", dest, 0);
            instr.srcVar = src1;
            instr.destVar = src2;
            result.push_back(instr);
        } else if (cmd == "SUBTRACT") {
            std::string dest, src1, src2;
            line >> dest >> src1 >> src2;
            if (dest.empty() || src1.empty() || src2.empty()) { errorMsg = "Invalid SUBTRACT syntax"; return {}; }
            Instruction instr(InstructionType::SUBTRACT, "", dest, 0);
            instr.srcVar = src1;
            instr.destVar = src2;
            result.push_back(instr);
        } else if (cmd == "READ") {
            std::string var, addrStr;
            line >> var >> addrStr;
            if (var.empty() || addrStr.empty()) { errorMsg = "Invalid READ syntax"; return {}; }
            uint32_t addr = std::stoul(addrStr, nullptr, 0);
            Instruction instr(InstructionType::READ, "", var, 0);
            instr.memAddress = addr;
            result.push_back(instr);
        } else if (cmd == "WRITE") {
            std::string addrStr, src;
            line >> addrStr >> src;
            if (addrStr.empty() || src.empty()) { errorMsg = "Invalid WRITE syntax"; return {}; }
            uint32_t addr = std::stoul(addrStr, nullptr, 0);
            Instruction instr(InstructionType::WRITE, "", src, 0);
            instr.memAddress = addr;
            result.push_back(instr);
        } else if (cmd == "PRINT") {
            std::string msg;
            std::getline(line, msg);
            msg = std::regex_replace(msg, std::regex("^\\s*\\("), "");
            msg = std::regex_replace(msg, std::regex("\\)$"), "");
            Instruction instr(InstructionType::PRINT, msg);
            result.push_back(instr);
        }
    }
    return result;
}