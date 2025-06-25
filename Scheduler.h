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

// Process instruction types
enum class InstructionType {
    PRINT,
    DECLARE,
    ADD,
    SUBTRACT,
    SLEEP,
    FOR_START,
    FOR_END
};

// Process instruction structure
struct Instruction {
    InstructionType type;
    std::string msg;
    std::string varName;
    int value;
    std::vector<Instruction> forBody;
    int forIterations;
    
    Instruction(InstructionType t, const std::string& m = "", const std::string& var = "", int val = 0);
};

// Process states
enum class ProcessState {
    READY,
    RUNNING,
    FINISHED
};

// Process class
class Process {
public:
    std::string name;
    int id;
    ProcessState state;
    std::vector<Instruction> instructions;
    int currentInstruction;
    std::map<std::string, int> variables;
    int coreId;
    std::chrono::system_clock::time_point creationTime;
    std::chrono::system_clock::time_point finishTime;
    bool isFinished;
    std::vector<int> forStack;
    std::vector<int> forCounters;
    
    Process(const std::string& processName, int processId);
    void generateRandomInstructions(int minIns, int maxIns);
};

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
    // std::vector<Process> allProcesses;
    std::vector<std::unique_ptr<Process>> allProcesses;
    std::mutex schedulerMutex;
    bool isInitialized;
    bool isRunning;
    bool allProcessesFinishedMessageShown;
    int processCounter;
    std::chrono::system_clock::time_point startTime;
    int cpuTicks;
    
    void schedulingLoop();
    void roundRobinSchedule();
    void fcfsSchedule();
    void executeInstruction(CPUCore& core);
    void processGenerationLoop();
    
public:
    Scheduler();
    
    bool initialize();
    void schedulerStart();
    void schedulerStop();
    void addProcess(const std::string& processName);
    void printScreen();
    void screenProcess(const std::string& processName);
    void reportUtil();
    
    // Utility methods
    double calculateCPUUtilization();
    int getActiveCores();
    bool getIsRunning() const { return isRunning; }
    
    // Get process information for screen sessions
    Process* getProcess(const std::string& processName);
    //std::vector<Process>& getAllProcesses() { return allProcesses; }
    std::vector<std::unique_ptr<Process>>& getAllProcesses() { return allProcesses; }

};

// Global scheduler instance
extern Scheduler globalScheduler;

#endif