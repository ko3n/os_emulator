#ifndef PROCESS_H
#define PROCESS_H

#include <string>
#include <vector>
#include <map>
#include <chrono>
#include <optional>

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
    std::optional<std::chrono::system_clock::time_point> executedAt;
    
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
    size_t memStart = 0;
    size_t memEnd = 0;
    int memRequired;        // Total memory needed by process
    int currentMemoryPage;  // Current page being accessed
    bool hasMemory;         // Has page table been created
    
    Process(const std::string& processName, int processId);
    void generateRandomInstructions(int minIns, int maxIns);
    
    // Memory access simulation for demand paging
    void accessRandomMemory(); // Simulate memory access causing page faults
};

#endif