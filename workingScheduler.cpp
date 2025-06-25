#include <iostream>
#include <vector>
#include <queue>
#include <map>
#include <string>
#include <fstream>
#include <sstream>
#include <thread>
#include <mutex>
#include <chrono>
#include <random>
#include <iomanip>
#include <algorithm>

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

    Instruction(InstructionType t, const std::string& m = "", const std::string& var = "", int val = 0)
        : type(t), msg(m), varName(var), value(val), forIterations(0) {
    }
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
    std::vector<int> forStack; // Stack for nested FOR loops
    std::vector<int> forCounters; // Counters for FOR loops

    Process(const std::string& processName, int processId)
        : name(processName), id(processId), state(ProcessState::READY),
        currentInstruction(0), coreId(-1), isFinished(false) {
        creationTime = std::chrono::system_clock::now();
    }

    void generateRandomInstructions(int minIns, int maxIns) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> instructionDist(minIns, maxIns);
        std::uniform_int_distribution<> typeDist(0, 5);
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
                instructions.emplace_back(InstructionType::SLEEP, "", "",
                    std::uniform_int_distribution<>(1, 10)(gen));
                break;
            case InstructionType::FOR_START:
                if (i < numInstructions - 2) {
                    int iterations = std::uniform_int_distribution<>(2, 5)(gen);
                    Instruction forInstr(InstructionType::FOR_START, "", "", iterations);
                    instructions.push_back(forInstr);
                    // Add a simple instruction inside the loop
                    instructions.emplace_back(InstructionType::PRINT,
                        "\"Loop iteration from " + name + "\"");
                    instructions.emplace_back(InstructionType::FOR_END);
                    i += 2; // Skip the added instructions
                }
                break;
            }
        }
    }
};

// Configuration structure
struct Config {
    int numCpu = 4;
    std::string scheduler = "rr";
    int quantumCycles = 5;
    int batchProcessFreq = 1;
    int minIns = 1000;
    int maxIns = 2000;
    int delaysPerExec = 0;

    bool loadFromFile(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            return false;
        }

        std::string line;
        while (std::getline(file, line)) {
            std::istringstream iss(line);
            std::string key, value;
            if (iss >> key >> value) {
                if (key == "num-cpu") numCpu = std::stoi(value);
                else if (key == "scheduler") scheduler = value;
                else if (key == "quantum-cycles") quantumCycles = std::stoi(value);
                else if (key == "batch-process-freq") batchProcessFreq = std::stoi(value);
                else if (key == "min-ins") minIns = std::stoi(value);
                else if (key == "max-ins") maxIns = std::stoi(value);
                else if (key == "delays-per-exec") delaysPerExec = std::stoi(value);
            }
        }
        return true;
    }
};

// CPU Core class
class CPUCore {
public:
    int id;
    Process* currentProcess;
    bool isRunning;
    int currentQuantum;

    CPUCore(int coreId) : id(coreId), currentProcess(nullptr), isRunning(false), currentQuantum(0) {}
};

// Scheduler class
class Scheduler {
private:
    Config config;
    std::vector<CPUCore> cores;
    std::queue<Process*> readyQueue;
    std::vector<Process> allProcesses;
    std::mutex schedulerMutex;
    bool isInitialized;
    bool isRunning;
    int processCounter;
    std::chrono::system_clock::time_point startTime;
    int cpuTicks;

public:
    Scheduler() : isInitialized(false), isRunning(false), processCounter(0), cpuTicks(0) {}

    bool initialize(const std::string& configFile = "config.txt") {
        if (!config.loadFromFile(configFile)) {
            // Use default config
            std::cout << "Config file not found, using default parameters.\n";
        }

        cores.clear();
        for (int i = 0; i < config.numCpu; i++) {
            cores.emplace_back(i);
        }

        isInitialized = true;
        startTime = std::chrono::system_clock::now();
        std::cout << "Scheduler initialized with " << config.numCpu << " CPU cores.\n";
        std::cout << "Scheduler algorithm: " << config.scheduler << "\n";
        std::cout << "Quantum cycles: " << config.quantumCycles << "\n";
        return true;
    }

    void schedulerStart() {
        if (!isInitialized) {
            std::cout << "Please initialize the scheduler first.\n";
            return;
        }

        isRunning = true;
        std::cout << "Scheduler started.\n";

        // Start the scheduling thread
        std::thread schedulingThread(&Scheduler::schedulingLoop, this);
        schedulingThread.detach();

        // Start process generation thread
        std::thread processGenThread(&Scheduler::processGenerationLoop, this);
        processGenThread.detach();
    }

    void schedulerStop() {
        isRunning = false;
        std::cout << "Scheduler stopped.\n";
    }

    void addProcess(const std::string& processName) {
        std::lock_guard<std::mutex> lock(schedulerMutex);

        allProcesses.emplace_back(processName, processCounter++);
        Process& newProcess = allProcesses.back();
        newProcess.generateRandomInstructions(config.minIns, config.maxIns);

        readyQueue.push(&newProcess);
        std::cout << "Process " << processName << " added to ready queue.\n";
    }

    void printScreen() {
        std::lock_guard<std::mutex> lock(schedulerMutex);

        std::cout << "CSOPESY\n";
        std::cout << "Welcome to CSOPESY Emulator!\n\n";

        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        std::cout << "Last updated: " << std::put_time(std::localtime(&time_t), "%m/%d/%Y") << "\n\n";

        std::cout << "CPU utilization: " << calculateCPUUtilization() << "%\n";
        std::cout << "Cores used: " << getActiveCores() << "\n";
        std::cout << "Cores available: " << (config.numCpu - getActiveCores()) << "\n\n";

        std::cout << "----------------------------------------\n";
        std::cout << "Running processes:\n";
        for (const auto& core : cores) {
            if (core.currentProcess) {
                auto process = core.currentProcess;
                auto currentTime = std::chrono::system_clock::now();
                auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                    currentTime - process->creationTime).count();
                std::cout << "process" << std::setfill('0') << std::setw(2) << process->id
                    << " (" << elapsed / 1000.0 << "s) Core: " << core.id
                    << " " << process->currentInstruction << " / " << process->instructions.size() << "\n";
            }
        }

        std::cout << "\nFinished processes:\n";
        for (const auto& process : allProcesses) {
            if (process.isFinished) {
                auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                    process.finishTime - process.creationTime).count();
                std::cout << "process" << std::setfill('0') << std::setw(2) << process.id
                    << " (" << elapsed / 1000.0 << "s) Finished " << elapsed / 1000.0 << "s\n";
            }
        }

        std::cout << "----------------------------------------\n";
    }

    void screenProcess(const std::string& processName) {
        std::lock_guard<std::mutex> lock(schedulerMutex);

        // Find process by name
        for (const auto& process : allProcesses) {
            if (process.name == processName) {
                std::cout << "Process name: " << process.name << "\n";
                std::cout << "ID: " << process.id << "\n";

                auto currentTime = std::chrono::system_clock::now();
                auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                    currentTime - process.creationTime).count();
                std::cout << "Current instruction line: " << process.currentInstruction << "\n";
                std::cout << "Lines of code: " << process.instructions.size() << "\n\n";

                if (process.isFinished) {
                    std::cout << "Finished!\n";
                }
                else {
                    std::cout << "Current instruction line: " << process.currentInstruction << "\n";
                }

                std::cout << "\nroot:\\> process-smi\n";
                return;
            }
        }

        std::cout << "Process " << processName << " not found.\n";
    }

    void reportUtil() {
        std::lock_guard<std::mutex> lock(schedulerMutex);

        std::ofstream report("csopesy-log.txt");
        if (report.is_open()) {
            report << "CPU Utilization Report\n";
            report << "CPU utilization: " << calculateCPUUtilization() << "%\n";
            report << "Cores used: " << getActiveCores() << "\n";
            report << "Cores available: " << (config.numCpu - getActiveCores()) << "\n\n";

            report << "Running processes:\n";
            for (const auto& core : cores) {
                if (core.currentProcess) {
                    auto process = core.currentProcess;
                    auto currentTime = std::chrono::system_clock::now();
                    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                        currentTime - process->creationTime).count();
                    report << "process" << std::setfill('0') << std::setw(2) << process->id
                        << " (" << elapsed / 1000.0 << "s) Core: " << core.id
                        << " " << process->currentInstruction << " / " << process->instructions.size() << "\n";
                }
            }

            report << "\nFinished processes:\n";
            for (const auto& process : allProcesses) {
                if (process.isFinished) {
                    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                        process.finishTime - process.creationTime).count();
                    report << "process" << std::setfill('0') << std::setw(2) << process.id
                        << " (" << elapsed / 1000.0 << "s) Finished " << elapsed / 1000.0 << "s\n";
                }
            }

            report.close();
            std::cout << "Report generated at csopesy-log.txt\n";
        }
    }

private:
    void schedulingLoop() {
        while (isRunning) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            cpuTicks++;

            std::lock_guard<std::mutex> lock(schedulerMutex);

            // Check for finished processes and free cores
            for (auto& core : cores) {
                if (core.currentProcess && core.currentProcess->isFinished) {
                    core.currentProcess = nullptr;
                    core.isRunning = false;
                    core.currentQuantum = 0;
                }
            }

            // Round-robin scheduling
            if (config.scheduler == "rr") {
                roundRobinSchedule();
            }

            // Execute instructions on running cores
            for (auto& core : cores) {
                if (core.currentProcess) {
                    executeInstruction(core);
                }
            }
        }
    }

    void roundRobinSchedule() {
        for (auto& core : cores) {
            if (!core.currentProcess && !readyQueue.empty()) {
                // Assign new process to core
                core.currentProcess = readyQueue.front();
                readyQueue.pop();
                core.currentProcess->state = ProcessState::RUNNING;
                core.currentProcess->coreId = core.id;
                core.isRunning = true;
                core.currentQuantum = 0;
            }
            else if (core.currentProcess && core.currentQuantum >= config.quantumCycles) {
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

    void executeInstruction(CPUCore& core) {
        if (!core.currentProcess || core.currentProcess->isFinished) return;

        Process* process = core.currentProcess;

        if (process->currentInstruction >= process->instructions.size()) {
            process->isFinished = true;
            process->state = ProcessState::FINISHED;
            process->finishTime = std::chrono::system_clock::now();
            return;
        }

        const Instruction& instr = process->instructions[process->currentInstruction];

        switch (instr.type) {
        case InstructionType::PRINT:
            // Simulate print execution
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
            // Simulate sleep by delaying execution
            std::this_thread::sleep_for(std::chrono::milliseconds(instr.value * 10));
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
                }
                else {
                    process->forStack.pop_back();
                    process->forCounters.pop_back();
                }
            }
            break;
        }

        process->currentInstruction++;

        // Add delay if configured
        if (config.delaysPerExec > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(config.delaysPerExec));
        }
    }

    void processGenerationLoop() {
        int processId = 0;
        while (isRunning) {
            std::this_thread::sleep_for(std::chrono::seconds(config.batchProcessFreq));

            if (isRunning) {
                std::string processName = "p" + std::to_string(processId++);
                addProcess(processName);
            }
        }
    }

    double calculateCPUUtilization() {
        int activeCores = getActiveCores();
        return (double)activeCores / config.numCpu * 100.0;
    }

    int getActiveCores() {
        int active = 0;
        for (const auto& core : cores) {
            if (core.currentProcess) active++;
        }
        return active;
    }
};

// Main console class
class Console {
private:
    Scheduler scheduler;
    bool isRunning;

public:
    Console() : isRunning(true) {}

    void run() {
        std::cout << "CSOPESY\n";
        std::cout << "Welcome to CSOPESY Emulator!\n\n";

        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        std::cout << "Last updated: " << std::put_time(std::localtime(&time_t), "%m/%d/%Y") << "\n\n";

        std::cout << "root:\\> ";

        std::string input;
        while (isRunning && std::getline(std::cin, input)) {
            processCommand(input);
            if (isRunning) {
                std::cout << "root:\\> ";
            }
        }
    }

private:
    void processCommand(const std::string& input) {
        std::istringstream iss(input);
        std::string command;
        iss >> command;

        if (command == "initialize") {
            scheduler.initialize();
        }
        else if (command == "scheduler-start") {
            scheduler.schedulerStart();
        }
        else if (command == "scheduler-stop") {
            scheduler.schedulerStop();
        }
        else if (command == "screen") {
            std::string option;
            if (iss >> option) {
                if (option == "-s") {
                    std::string processName;
                    if (iss >> processName) {
                        scheduler.screenProcess(processName);
                    }
                    else {
                        std::cout << "Usage: screen -s <process_name>\n";
                    }
                }
                else if (option == "-ls") {
                    scheduler.printScreen();
                }
                else {
                    scheduler.screenProcess(option);
                }
            }
            else {
                scheduler.printScreen();
            }
        }
        else if (command == "report-util") {
            scheduler.reportUtil();
        }
        else if (command == "exit") {
            scheduler.schedulerStop();
            isRunning = false;
        }
        else if (command == "clear") {
            system("clear || cls");
        }
        else if (!command.empty()) {
            std::cout << "Unknown command: " << command << "\n";
            std::cout << "Available commands: initialize, scheduler-start, scheduler-stop, screen, screen -s <process>, screen -ls, report-util, exit, clear\n";
        }
    }
};

int main() {
    Console console;
    console.run();
    return 0;
}