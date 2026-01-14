#include "Process.h"
#include <random>

// Instruction implementation
Instruction::Instruction(InstructionType t, const std::string& m, const std::string& var, int val)
    : type(t), msg(m), varName(var), value(val), srcVar(""), destVar(""), memAddress(0), forIterations(0), executedAt(std::nullopt) {}
// Process implementation
Process::Process(const std::string& processName, int processId) 
    : name(processName), id(processId), state(ProcessState::READY), 
      currentInstruction(0), coreId(-1), isFinished(false) {
    creationTime = std::chrono::system_clock::now();
}

void Process::generateRandomInstructions(int minIns, int maxIns) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> instructionDist(minIns, maxIns);
    std::uniform_int_distribution<> typeDist(0, 7);
    std::uniform_int_distribution<> valueDist(1, 100);
    
    int numInstructions = instructionDist(gen);
    
    for (int i = 0; i < numInstructions; i++) {
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
                instructions.emplace_back(InstructionType::SLEEP, "", "", 2);
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
                } else {
                    // Not enough space for FOR loop, add PRINT instead
                    instructions.emplace_back(InstructionType::PRINT, 
                    "\"Hello world from " + name + "!\"");
                }
                break;
            case InstructionType::READ: {
                std::string var = "var" + std::to_string(i % 32);
                uint32_t addr = (rand() % 0x10000) & ~1; // aligned, 16-bit
                instructions.emplace_back(InstructionType::READ, "", var, 0);
                instructions.back().memAddress = addr;
                break;
            }
            case InstructionType::WRITE: {
                uint32_t addr = (rand() % 0x10000) & ~1;
                std::string src = "var" + std::to_string(i % 32);
                instructions.emplace_back(InstructionType::WRITE, "", src, 0);
                instructions.back().memAddress = addr;
                break;
            }
        }
    }
}