#ifndef PROCESS_H
#define PROCESS_H

#include <string>
#include <vector>
#include <map>
#include <chrono>

enum class InstructionType {
    PRINT,
    DECLARE,
    ADD,
    SUBTRACT,
    SLEEP,
    FOR_START,
    FOR_END
};

struct Instruction {
    InstructionType type;
    std::string msg;
    std::string varName;
    int value;
    std::vector<Instruction> forBody;
    int forIterations;
    
    Instruction(InstructionType t, const std::string& m = "", const std::string& var = "", int val = 0);
};

enum class ProcessState {
    READY,
    RUNNING,
    FINISHED
};

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

#endif