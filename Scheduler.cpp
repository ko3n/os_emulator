#include "Scheduler.h"
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


// CPUCore implementation
CPUCore::CPUCore(int coreId) : id(coreId), currentProcess(nullptr), isRunning(false), currentQuantum(0) {}

// Scheduler implementation
Scheduler::Scheduler() :
    memoryManager(systemConfig.maxOverallMem, systemConfig.memPerProc),
    isInitialized(false),
    isRunning(false),
    isGeneratingProcesses(false),
    allProcessesFinishedMessageShown(false),
    processCounter(0),
    cpuTicks(0) {}

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
    process->generateRandomInstructions(systemConfig.minInstructions, systemConfig.maxInstructions); // Generate process instructions
    readyQueue.push(process.get()); // Push raw pointer to queue
    allProcesses.push_back(std::move(process)); // Transfer ownership to vector
    
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
            std::cout << std::left
                      << std::setw(12) << process->name
                      << std::setw(28) << "(waiting for memory)"
                      << std::setw(10) << ""
                      << std::setw(10) << ""
                      << "\n";
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
        cpuTicks++;
        {
            std::lock_guard<std::mutex> lock(schedulerMutex);

            // 1. Deallocate memory for finished processes and free cores
            for (auto& core : cores) {
                if (core.currentProcess && core.currentProcess->isFinished) {
                    if (core.currentProcess->hasMemory) {
                        memoryManager.free(core.currentProcess);
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
                    // Optionally print debug info
                    memoryManager.allocate(proc);
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

            // After all scheduling and execution logic, output memory snapshot every quantum cycle
            // Only output memory snapshot if not all processes are finished
            bool allFinished = true;
            for (const auto& processPtr : allProcesses) {
                if (!processPtr->isFinished) {
                    allFinished = false;
                    break;
                }
            }
            bool hasRunningProcesses = false;
            for (const auto& core : cores) {
                if (core.currentProcess) {
                    hasRunningProcesses = true;
                    break;
                }
            }
            bool readyQueueEmpty = readyQueue.empty();
            if (!allFinished || hasRunningProcesses || !readyQueueEmpty) {
                if (cpuTicks > 0 && ((cpuTicks - 1) % systemConfig.quantumCycles == 0)) {
                    int quantumNum = (cpuTicks - 1) / systemConfig.quantumCycles + 1;
                    outputMemorySnapshot(memoryManager, quantumNum);
                }
            }
        }
    }
}

void Scheduler::roundRobinSchedule() {
    for (auto& core : cores) {
        if (!core.currentProcess && !readyQueue.empty()) {
            // Assign new process to core
            core.currentProcess = readyQueue.front();
            readyQueue.pop();
            core.currentProcess->state = ProcessState::RUNNING;
            core.currentProcess->coreId = core.id;
            core.isRunning = true;
            core.currentQuantum = 0;
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
                core.currentProcess = readyQueue.front();
                readyQueue.pop();
                core.currentProcess->state = ProcessState::RUNNING;
                core.currentProcess->coreId = core.id;
                core.isRunning = true;
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

            nextProc->state = ProcessState::RUNNING;
            nextProc->coreId = core.id;

            core.currentProcess = nextProc;
            core.isRunning = true;
            //No quantum bookkeeping needed for FCFS
        }
    }
}

void Scheduler::executeInstruction(CPUCore& core) {
    if (!core.currentProcess || core.currentProcess->isFinished) return;
    
    Process* process = core.currentProcess;
    
    if (process->currentInstruction >= process->instructions.size()) {
        process->isFinished = true;
        process->state = ProcessState::FINISHED;
        process->finishTime = std::chrono::system_clock::now();
        // std::cout << "\n" << process->name << " has finished execution.\n>";
        return;
    }
    
    // const Instruction& instr = process->instructions[process->currentInstruction];
    Instruction& instr = process->instructions[process->currentInstruction];
    instr.executedAt = std::chrono::system_clock::now();
    
    switch (instr.type) {
        case InstructionType::PRINT:
            break;
        case InstructionType::DECLARE:
            process->variables[instr.varName] = instr.value;
            break;
        case InstructionType::ADD:
            if (process->variables.find(instr.varName) != process->variables.end()) {
                process->variables[instr.varName] += instr.value;
            }
            break;
        case InstructionType::SUBTRACT:
            if (process->variables.find(instr.varName) != process->variables.end()) {
                process->variables[instr.varName] -= instr.value;
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
        if (core.currentProcess) active++;
    }
    return active;
}