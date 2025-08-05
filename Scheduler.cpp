#include "Scheduler.h"
#include "Process.h"
#include "Config.h"
#include "MemoryManager.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <random>
#include <algorithm>
#include <filesystem>
#include <ctime>
#include <cstdlib>  // Add this for rand()


// CPUCore implementation
CPUCore::CPUCore(int coreId) : id(coreId), currentProcess(nullptr), isRunning(false), currentQuantum(0) {}

// Scheduler implementation
Scheduler::Scheduler() :
    memoryManager(systemConfig.maxOverallMem, systemConfig.memPerFrame),
    isInitialized(false),
    isRunning(false),
    isGeneratingProcesses(false),
    allProcessesFinishedMessageShown(false),
    processCounter(0),
    cpuTicks(0),
    totalCpuTicks(0),
    activeCpuTicks(0),
    idleCpuTicks(0) {}

bool Scheduler::initialize() {
    cores.clear();
    for (int i = 0; i < systemConfig.numCPU; i++) {
        cores.emplace_back(i);
    }
    
    isInitialized = true;
    startTime = std::chrono::system_clock::now();
    std::cout << "Scheduler initialized with " << systemConfig.numCPU << " CPU cores.\n";
    std::cout << "Scheduler algorithm: " << systemConfig.scheduler << "\n";
    std::cout << "Quantum cycles: " << systemConfig.quantumCycles << "\n";

    // Set up the link between memory manager and scheduler for statistics
    memoryManager.setScheduler(this);
    
    return true;
}

void Scheduler::schedulerTest() {
    if (!isInitialized) {
        std::cout << "Please initialize the scheduler first.\n";
        return;
    }
    
    isRunning = true;
    isGeneratingProcesses = true;
    allProcessesFinishedMessageShown = false; // Reset flag when starting
    std::cout << "Scheduler started.\n";
    
    // COMMENTED OUT FOR NOW
    // Immediately create 4 processes at startup 
    // for (int i = 0; i < 4; ++i) {
    //    std::string processName = "process" + std::to_string(i);
    //    addProcess(processName);
    //}

    std::thread schedulingThread(&Scheduler::schedulingLoop, this);
    schedulingThread.detach();

    // Continue generating more processes as before
    std::thread processGenThread(&Scheduler::processGenerationLoop, this);
    processGenThread.detach();
}

void Scheduler::schedulerStop() {
    isGeneratingProcesses = false;
    std::cout << "Scheduler stopped.\n";
}

void Scheduler::addProcess(const std::string& processName) {
    std::lock_guard<std::mutex> lock(schedulerMutex);

    auto process = std::make_unique<Process>(processName, processCounter++);
    process->generateRandomInstructions(systemConfig.minInstructions, systemConfig.maxInstructions);
    
    // Assign random memory size between min and max
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(systemConfig.minMemPerProc, systemConfig.maxMemPerProc);
    process->memRequired = dis(gen);
    
    // Store memory requirement before moving the process
    int memReq = process->memRequired;
    
    // Allocate process (creates page table, but doesn't load pages yet)
    if (memoryManager.allocateProcess(process.get())) {
        readyQueue.push(process.get());
        allProcesses.push_back(std::move(process));
        // std::cout << "[Scheduler] Process " << processName << " created with " 
        //           << memReq << " bytes memory requirement\n";
    } else {
        std::cout << "[Scheduler] Failed to allocate process " << processName << "\n";
    }
}

void Scheduler::addProcessWithMemory(const std::string& processName, int memorySize, const std::vector<Instruction>& userInstructions) {
    std::lock_guard<std::mutex> lock(schedulerMutex);
    auto process = std::make_unique<Process>(processName, processCounter++);
    process->instructions = userInstructions; 
    process->memRequired = memorySize;
    if (memoryManager.allocateProcess(process.get())) {
        readyQueue.push(process.get());
        allProcesses.push_back(std::move(process));
        std::cout << "[Scheduler] Process " << processName << " allocated with " << memorySize << " bytes of memory.\n";
    } else {
        std::cout << "[Scheduler] Failed to allocate memory for process " << processName << " - insufficient memory available.\n";
    }
}

void Scheduler::printScreen() {
    std::lock_guard<std::mutex> lock(schedulerMutex);
    
    std::cout << "\n";    
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::cout << "Last updated: " << std::put_time(std::localtime(&time_t), "%m/%d/%Y") << "\n\n";
    
    std::cout << "CPU utilization: " << calculateCPUUtilization() << "%\n";
    std::cout << "Cores used: " << getActiveCores() << "\n";
    std::cout << "Cores available: " << (systemConfig.numCPU - getActiveCores()) << "\n\n";
    
    std::cout << "----------------------------------------\n";
    std::cout << "Running processes:\n";
    for (const auto& core : cores) {
        if (core.currentProcess) {
            auto process = core.currentProcess;
            auto currentTime = std::chrono::system_clock::now();
            auto time_t = std::chrono::system_clock::to_time_t(process->creationTime);
            
            // Format timestamp into string
            std::ostringstream timeStream;
            timeStream << "(" << std::put_time(std::localtime(&time_t), "%m/%d/%Y %I:%M:%S %p") << ")";
            std::string timeStr = timeStream.str();

            // Format core info
            std::string coreStr = "Core: " + std::to_string(core.id);

            // Format instruction progress
            std::string progressStr = std::to_string(process->currentInstruction) + " / " + std::to_string(process->instructions.size());

            std::cout << std::left
                      << std::setw(12) << process->name
                      << std::setw(28) << timeStr
                      << std::setw(10) << coreStr
                      << std::setw(10) << progressStr
                      << "\n";
        }
    }
    
    std::cout << "\nIn queue:\n";
    
    // Show processes waiting for memory
    for (const auto& processPtr : allProcesses) {
        const Process* process = processPtr.get();
        if (process->isFinished) continue;
        
        // Check if process is running on a core
        bool isRunning = false;
        for (const auto& core : cores) {
            if (core.currentProcess == process) {
                isRunning = true;
                break;
            }
        }
        
        if (!isRunning) {
            if (!process->hasMemory) {
                // Process waiting for memory
                std::cout << std::left
                          << std::setw(12) << process->name
                          << std::setw(28) << "(Waiting for memory)"
                          << std::setw(10) << ""
                          << std::setw(10) << ""
                          << "\n";
            } else {
                // Process has memory and is in ready queue
                std::cout << std::left
                          << std::setw(12) << process->name
                          << std::setw(28) << "(Ready)"
                          << std::setw(10) << ""
                          << std::setw(10) << ""
                          << "\n";
            }
        }
    }

    std::cout << "\nFinished processes:\n";

    std::vector<const Process*> finishedProcesses;
    for (const auto& processPtr : allProcesses) {
        if (processPtr->isFinished) {
            finishedProcesses.push_back(processPtr.get());
        }
    }

    std::sort(finishedProcesses.begin(), finishedProcesses.end(), [](const Process* a, const Process* b) {
        return a->finishTime < b->finishTime;
    });

    for (const Process* process : finishedProcesses) {
        auto time_t = std::chrono::system_clock::to_time_t(process->finishTime);

        // Format timestamp into string 
        std::ostringstream timeStream;
        timeStream << "(" << std::put_time(std::localtime(&time_t), "%m/%d/%Y %I:%M:%S %p") << ")";
        std::string timeStr = timeStream.str();

        // Format instruction progress
        std::string progressStr = std::to_string(process->instructions.size()) + " / " + std::to_string(process->instructions.size());

        std::cout << std::left
                  << std::setw(12) << process->name
                  << std::setw(28) << timeStr
                  << std::setw(12) << "Finished"
                  << std::setw(10) << progressStr
                  << "\n";        
    }

    std::cout << "----------------------------------------\n";
}

void Scheduler::reportUtil() {
    std::lock_guard<std::mutex> lock(schedulerMutex);
    
    std::ofstream report("csopesy-log.txt");
    if (report.is_open()) {
        report << "CPU Utilization Report\n";
        report << "CPU utilization: " << calculateCPUUtilization() << "%\n";
        report << "Cores used: " << getActiveCores() << "\n";
        report << "Cores available: " << (systemConfig.numCPU - getActiveCores()) << "\n\n";
        
        report << "Running processes:\n";
        for (const auto& core : cores) {
            if (core.currentProcess) {
                auto process = core.currentProcess;
                auto currentTime = std::chrono::system_clock::now();
                auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                    currentTime - process->creationTime).count();
                report << process->name << "   " 
                      << " (" << elapsed/1000.0 << "s)   " << "Core: " << core.id 
                      << "   " << process->currentInstruction << " / " << process->instructions.size() << "\n";
            }
        }
        
        // Show waiting for memory processes (skip those running on a core)
        for (const auto& processPtr : allProcesses) {
            const Process* process = processPtr.get();
            if (process->isFinished) continue;
            bool isRunning = false;
            for (const auto& core : cores) {
                if (core.currentProcess == process) {
                    isRunning = true;
                    break;
                }
            }
            if (!isRunning && !process->hasMemory) {
                report << std::left
                       << std::setw(12) << process->name
                       << std::setw(28) << "(waiting for memory)"
                       << std::setw(10) << ""
                       << std::setw(10) << ""
                       << "\n";
            }
        }

        report << "\nFinished processes:\n";

        std::vector<const Process*> finishedProcesses;
        for (const auto& processPtr : allProcesses) {
            if (processPtr->isFinished) {
                finishedProcesses.push_back(processPtr.get());
            }
        }

        std::sort(finishedProcesses.begin(), finishedProcesses.end(), [](const Process* a, const Process* b) {
            return a->finishTime < b->finishTime;
        });

        for (const Process* process : finishedProcesses) {
            auto time_t = std::chrono::system_clock::to_time_t(process->finishTime);

            // Format timestamp into string
            std::ostringstream timeStream;
            timeStream << "(" << std::put_time(std::localtime(&time_t), "%m/%d/%Y %I:%M:%S %p") << ")";
            
            report << std::left
                   << std::setw(12) << process->name
                   << std::setw(28) << timeStream.str()
                   << std::setw(12) << "Finished"
                   << std::setw(10) << (std::to_string(process->instructions.size()) + " / " + std::to_string(process->instructions.size()))
                   << "\n";
        }
        
        report.close();
        //std::cout << "Report generated at csopesy-log.txt\n";
        std::filesystem::path filePath = std::filesystem::absolute("csopesy-log.txt");
        std::cout << "Report generated at: " << filePath << "\n";
    }
}

Process* Scheduler::getProcess(const std::string& processName) {
    std::lock_guard<std::mutex> lock(schedulerMutex);
    
    for (auto& processPtr : allProcesses) {
        if (processPtr->name == processName) {
            return processPtr.get();
        }
    }
    return nullptr;
}

void Scheduler::schedulingLoop() {
    while (isRunning) {
        std::this_thread::sleep_for(std::chrono::milliseconds(18));
        
        // Increment total CPU ticks for each cycle
        totalCpuTicks++;
        
        {
            std::lock_guard<std::mutex> lock(schedulerMutex);

            // Count currently active cores
            int currentActiveCores = 0;
            for (const auto& core : cores) {
                if (core.currentProcess) {
                    currentActiveCores++;
                }
            }
            
            // Update CPU statistics
            activeCpuTicks += currentActiveCores;
            idleCpuTicks += (systemConfig.numCPU - currentActiveCores);

            // 1. Deallocate memory for finished processes and free cores
            for (auto& core : cores) {
                if (core.currentProcess && core.currentProcess->isFinished) {
                    if (core.currentProcess->hasMemory) {
                        memoryManager.deallocateProcess(core.currentProcess);
                    }
                    core.currentProcess = nullptr;
                    core.isRunning = false;
                    core.currentQuantum = 0;
                }
            }

            // 2. Try to allocate memory for processes in the ready queue that don't have memory
            std::queue<Process*> tempQueue;
            while (!readyQueue.empty()) {
                Process* proc = readyQueue.front();
                readyQueue.pop();
                if (!proc->hasMemory && !proc->isFinished) {
                    memoryManager.allocateProcess(proc); // Fixed method name
                }
                tempQueue.push(proc);
            }
            readyQueue = tempQueue;

            // 3. Schedule processes
            if (systemConfig.scheduler == "rr") {
                roundRobinSchedule();
            }
            else if (systemConfig.scheduler == "fcfs") {
                fcfsSchedule();
            }

            // 4. Execute instructions on running cores
            for (auto& core : cores) {
                if (core.currentProcess) {
                    executeInstruction(core);
                }
            }
            
            // Check if all processes are finished
            if (!allProcessesFinishedMessageShown && !allProcesses.empty()) {
                bool allFinished = true;
                bool hasRunningProcesses = false;
                
                // Check if any process is still running or in ready queue
                for (const auto& processPtr : allProcesses) {
                    const Process& process = *processPtr;
                    if (!process.isFinished) {
                        allFinished = false;
                        break;
                    }
                }
                
                // Check if any core has a running process
                for (const auto& core : cores) {
                    if (core.currentProcess) {
                        hasRunningProcesses = true;
                        break;
                    }
                }
                
                // Check if ready queue is empty
                bool readyQueueEmpty = readyQueue.empty();
                
                if (allFinished && !hasRunningProcesses && readyQueueEmpty) {
                    std::cout << "\n=== All processes have finished execution ===\n";
                    std::cout << "Scheduler is still running. Use 'screen -ls' to view process summary.\n";
                    std::cout << "Type 'scheduler-stop' to stop the scheduler or 'exit' to quit.\n\n>";
                    allProcessesFinishedMessageShown = true;
                }
            }

            // Remove the memory snapshot output for now since we're focusing on core demand paging
            // We can add this back later when implementing process-smi and vmstat
        }
    }
}

void Scheduler::roundRobinSchedule() {
    for (auto& core : cores) {
        if (!core.currentProcess && !readyQueue.empty()) {
            // Assign new process to core
            Process *nextProc = readyQueue.front();
            readyQueue.pop();
            // Only schedule if process has memory
            if (nextProc->hasMemory && !nextProc->isFinished) {
                core.currentProcess = nextProc;
                nextProc->state = ProcessState::RUNNING;
                nextProc->coreId = core.id;
                core.isRunning = true;
                core.currentQuantum = 0;
            } else {
                // Put back in queue if no memory
                readyQueue.push(nextProc);
            }
        } else if (core.currentProcess && core.currentQuantum >= systemConfig.quantumCycles) {
            // Time slice expired, preempt process
            if (!core.currentProcess->isFinished) {
                core.currentProcess->state = ProcessState::READY;
                readyQueue.push(core.currentProcess);
            }
            core.currentProcess = nullptr;
            core.isRunning = false;
            core.currentQuantum = 0;
            
            // Assign new process if available
            if (!readyQueue.empty()) {
                Process* nextProc = readyQueue.front();
                readyQueue.pop();
                if (nextProc->hasMemory && !nextProc->isFinished) {
                    core.currentProcess = nextProc;
                    nextProc->state = ProcessState::RUNNING;
                    nextProc->coreId = core.id;
                    core.isRunning = true;
                } else {
                    readyQueue.push(nextProc);
                }
            }
        }
        
        if (core.currentProcess) {
            core.currentQuantum++;
        }
    }
}

void Scheduler::fcfsSchedule(){
    for(auto &core : cores){
        if(!core.currentProcess && !readyQueue.empty()){
            Process* nextProc = readyQueue.front();
            readyQueue.pop();

            // Only schedule if process has memory
            if (nextProc->hasMemory && !nextProc->isFinished) {
                nextProc->state = ProcessState::RUNNING;
                nextProc->coreId = core.id;
                core.currentProcess = nextProc;
                core.isRunning = true;
            } else {
                // Put back in queue if no memory
                readyQueue.push(nextProc);
            }
        }
    }
}

void Scheduler::executeInstruction(CPUCore& core) {
    if (!core.currentProcess || core.currentProcess->isFinished) return;
    Process* process = core.currentProcess;
    
    // Add memory access simulation that may cause page faults
    if (process->hasMemory) {
        int virtualAddr = rand() % process->memRequired;
        memoryManager.accessMemory(process, virtualAddr);
    }
    
    if (process->currentInstruction >= process->instructions.size()) {
        process->isFinished = true;
        process->state = ProcessState::FINISHED;
        process->finishTime = std::chrono::system_clock::now();
        // Deallocate memory when process finishes
        if (process->hasMemory) {
            memoryManager.deallocateProcess(process);
        }
        return;
    }
    
    Instruction& instr = process->instructions[process->currentInstruction];
    instr.executedAt = std::chrono::system_clock::now();
    switch (instr.type) {
        case InstructionType::PRINT: {
            // Enhanced PRINT: substitute $varName with variable value
            std::string msg = instr.msg;
            size_t pos = 0;
            while ((pos = msg.find('$', pos)) != std::string::npos) {
                size_t end = pos + 1;
                while (end < msg.size() && (std::isalnum(msg[end]) || msg[end] == '_')) ++end;
                std::string varName = msg.substr(pos + 1, end - pos - 1);
                if (!varName.empty() && process->variables.count(varName)) {
                    msg.replace(pos, end - pos, std::to_string(process->variables[varName]));
                    pos += std::to_string(process->variables[varName]).size();
                } else {
                    pos = end;
                }
            }
            // Optionally, print to console here if you want PRINT to always show (not required for screen session)
            // std::cout << msg << std::endl;
            instr.msg = msg; // Store the substituted message for screen session
            break;
        }
        case InstructionType::DECLARE:
            process->variables[instr.varName] = instr.value;
            instr.msg = "DECLARE: " + instr.varName + " = " + std::to_string(instr.value);
            break;
        case InstructionType::ADD:
            if (process->variables.find(instr.srcVar) != process->variables.end() && process->variables.find(instr.destVar) != process->variables.end()) {
                int val1 = process->variables[instr.srcVar];
                int val2 = process->variables[instr.destVar];
                int result = val1 + val2;
                process->variables[instr.varName] = result;
                std::ostringstream oss;
                oss << "ADD: " << val1 << " + " << val2 << " = " << result << " (" << instr.varName << " = " << result << ")";
                instr.msg = oss.str();
            }
            break;
        case InstructionType::SUBTRACT:
            if (process->variables.find(instr.srcVar) != process->variables.end() && process->variables.find(instr.destVar) != process->variables.end()) {
                int val1 = process->variables[instr.srcVar];
                int val2 = process->variables[instr.destVar];
                int result = val1 - val2;
                process->variables[instr.varName] = result;
                std::ostringstream oss;
                oss << "SUBTRACT: " << val1 << " - " << val2 << " = " << result << " (" << instr.varName << " = " << result << ")";
                instr.msg = oss.str();
            }
            break;
        case InstructionType::SLEEP:
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
            break;
        case InstructionType::FOR_START:
            process->forStack.push_back(process->currentInstruction);
            process->forCounters.push_back(0);
            break;
        case InstructionType::FOR_END:
            if (!process->forStack.empty()) {
                int forStart = process->forStack.back();
                int& counter = process->forCounters.back();
                counter++;
                if (counter < process->instructions[forStart].value) {
                    process->currentInstruction = forStart;
                } else {
                    process->forStack.pop_back();
                    process->forCounters.pop_back();
                }
            }
            break;
        case InstructionType::READ: {
            // Check symbol table size
            if (process->variables.size() >= 32 && process->variables.find(instr.varName) == process->variables.end()) break;
            uint16_t val = 0;
            int physAddr = 0;
            if (memoryManager.translateAddress(process, instr.memAddress, physAddr)) {
                // Simulate memory read (assume physicalMemory is accessible)
                val = memoryManager.readWord(physAddr);
            }
            process->variables[instr.varName] = val;
            // Improved log message: show variable and address only (no value)
            std::ostringstream oss;
            oss << "READ: " << instr.varName << " <- [0x" << std::hex << std::uppercase << instr.memAddress << "]";
            instr.msg = oss.str();
            break;
        }
        case InstructionType::WRITE: {
            int physAddr = 0;
            uint16_t val = 0;
            // Use instr.varName as the variable to write
            std::string srcVarName = instr.varName;
            if (process->variables.count(srcVarName)) val = (uint16_t)process->variables[srcVarName];
            if (memoryManager.translateAddress(process, instr.memAddress, physAddr)) {
                memoryManager.writeWord(physAddr, val);
            }
            // Log: show value and address (e.g., 'WRITE: 15 to 0x500')
            std::ostringstream oss;
            oss << "WRITE: " << val << " to 0x" << std::hex << std::uppercase << instr.memAddress;
            instr.msg = oss.str();
            break;
        }
    }

    process->currentInstruction++;

    if (systemConfig.delayPerExec > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(systemConfig.delayPerExec));
    }
}

void Scheduler::processGenerationLoop() {
    int automaticProcessCounter = 0; // separate counter for dummy processes

    while (isGeneratingProcesses) {
        std::this_thread::sleep_for(std::chrono::seconds(systemConfig.batchProcessFreq));
        
        if (isGeneratingProcesses) {
            std::string processName = "process" + std::to_string(automaticProcessCounter);
            automaticProcessCounter++;
            addProcess(processName);
        }
    }
}

double Scheduler::calculateCPUUtilization() {
    int activeCores = getActiveCores();
    return (double)activeCores / systemConfig.numCPU * 100.0;
}

int Scheduler::getActiveCores() {
    int active = 0;
    for (const auto& core : cores) {
        if (core.currentProcess) {
            active++;
        }
    }
    return active;
}