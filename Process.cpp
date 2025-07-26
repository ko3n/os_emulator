#include "Process.h"
#include <random>

// Instruction implementation
Instruction::Instruction(InstructionType t, const std::string& m, const std::string& var, int val) 
    : type(t), msg(m), varName(var), value(val), forIterations(0), executedAt(std::nullopt) {}

// Process implementation
Process::Process(const std::string& processName, int processId) 
    : name(processName), id(processId), state(ProcessState::READY), 
      currentInstruction(0), coreId(-1), isFinished(false) {
    creationTime = std::chrono::system_clock::now();
}

// Instruction generation
void Process::generateRandomInstructions(int minIns, int maxIns) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> instructionDist(minIns, maxIns);
    std::uniform_int_distribution<> typeDist(0, 5);
    std::uniform_int_distribution<> valueDist(1, 100);
    // std::uniform_int_distribution<> addValueDist(1, 10);
    
    int numInstructions = instructionDist(gen);

    // Declare variable "x" with value 0
    // instructions.emplace_back(InstructionType::DECLARE, "", "x", 0);
    
    // Minus 1 from numInstructions because we have 1 DECLARE instruction outside this loop
    // for (int i = 0; i < numInstructions - 1; i++) {
    for (int i = 0; i < numInstructions; i++) {

        /*if (i % 2 == 1) {
            // Odd indices: PRINT 
            instructions.emplace_back(InstructionType::PRINT, 
                "Value from: x", "x"); 
        } else {
            // Even indices: ADD 
            int addValue = addValueDist(gen);
            instructions.emplace_back(InstructionType::ADD, "", "x", addValue);
        }*/

        InstructionType type = static_cast<InstructionType>(typeDist(gen));
        
        switch (type) {
            case InstructionType::PRINT:
                instructions.emplace_back(InstructionType::PRINT, 
                    "\"Hello world from " + name + "!\"");
                break;
            case InstructionType::DECLARE:
                instructions.emplace_back(InstructionType::DECLARE, "", 
                    "var" + std::to_string(i), valueDist(gen));
                break;
            case InstructionType::ADD:
                instructions.emplace_back(InstructionType::ADD, "", 
                    "var" + std::to_string(i % 3), valueDist(gen));
                break;
            case InstructionType::SUBTRACT:
                instructions.emplace_back(InstructionType::SUBTRACT, "", 
                    "var" + std::to_string(i % 3), valueDist(gen));
                break;
            case InstructionType::SLEEP:
                instructions.emplace_back(InstructionType::SLEEP, "", "", 
                    std::uniform_int_distribution<>(1, 10)(gen));
                break;
            case InstructionType::FOR_START:
                if (i < numInstructions - 2) {
                    int iterations = std::uniform_int_distribution<>(2, 5)(gen);
                    Instruction forInstr(InstructionType::FOR_START, "", "", iterations);
                    instructions.push_back(forInstr);
                    instructions.emplace_back(InstructionType::PRINT, 
                        "\"Hello world from " + name + "!\"");
                    instructions.emplace_back(InstructionType::FOR_END);
                    i += 2;
                }
                break;
        }
    }
}