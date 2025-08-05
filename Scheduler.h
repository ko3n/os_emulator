#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <string>
#include <vector>
#include <map>
#include <queue>
#include <mutex>
#include <thread>
#include <chrono>
#include <memory>
#include "Process.h"
#include "MemoryManager.h"

// CPU Core class
class CPUCore {
public:
    int id;
    Process* currentProcess;
    bool isRunning;
    int currentQuantum;
    
    CPUCore(int coreId);
};

// Scheduler class
class Scheduler {
private:
    std::vector<CPUCore> cores;
    std::queue<Process*> readyQueue;
    std::vector<std::unique_ptr<Process>> allProcesses;
    std::mutex schedulerMutex;
    bool isInitialized;
    bool isRunning;
    bool isGeneratingProcesses;
    bool allProcessesFinishedMessageShown;
    int processCounter;
    std::chrono::system_clock::time_point startTime;
    int cpuTicks;
    MemoryManager memoryManager;
    
    // Add these statistics tracking variables:
    long long idleCpuTicks = 0;
    long long activeCpuTicks = 0;
    long long totalCpuTicks = 0;
    long long numPagedIn = 0;
    long long numPagedOut = 0;
    
    void schedulingLoop();
    void roundRobinSchedule();
    void fcfsSchedule();
    void executeInstruction(CPUCore& core);
    void processGenerationLoop();
    
public:
    Scheduler();
    
    bool initialize();
    void schedulerTest();
    void schedulerStop();
    void addProcess(const std::string& processName);
    
    void addProcessWithMemory(const std::string& processName, int memorySize, const std::vector<Instruction>& userInstructions);
    
    void printScreen();
    void screenProcess(const std::string& processName);
    void reportUtil();
    
    // Utility methods
    double calculateCPUUtilization();
    int getActiveCores();
    bool getIsRunning() const { return isRunning; }
    
    // Get process information for screen sessions
    Process* getProcess(const std::string& processName);
    std::vector<std::unique_ptr<Process>>& getAllProcesses() { return allProcesses; }
    MemoryManager& getMemoryManager() { return memoryManager; }
    
    // Add getters for vmstat statistics:
    long long getIdleCpuTicks() const { return idleCpuTicks; }
    long long getActiveCpuTicks() const { return activeCpuTicks; }
    long long getTotalCpuTicks() const { return totalCpuTicks; }
    long long getNumPagedIn() const { return numPagedIn; }
    long long getNumPagedOut() const { return numPagedOut; }
    
    // Methods to increment paging stats (called by MemoryManager)
    void incrementPagedIn() { numPagedIn++; }
    void incrementPagedOut() { numPagedOut++; }
    
    void processSmi();
    
    void startIfNotRunning() {
        if (!isRunning) {
            schedulerTest();
        }
    }
    
    void startSchedulingLoopOnly();
};

#endif