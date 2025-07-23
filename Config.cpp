#include "Config.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <iomanip>

Config systemConfig;

bool loadConfig(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Failed to open " << filename << "\n";
        return false;
    }

    std::string key;
    while (file >> key) {
        if (key == "num-cpu") {
            file >> systemConfig.numCPU;
        } else if (key == "scheduler") {
            std::string sched;
            file >> std::quoted(sched);  // Reads quoted string 
            systemConfig.scheduler = sched;
        } else if (key == "quantum-cycles") {
            file >> systemConfig.quantumCycles;
        } else if (key == "batch-process-freq") {
            file >> systemConfig.batchProcessFreq;
        } else if (key == "min-ins") {
            file >> systemConfig.minInstructions;
        } else if (key == "max-ins") {
            file >> systemConfig.maxInstructions;
        } else if (key == "delay-per-exec") {
            file >> systemConfig.delayPerExec;
        // MO2 Parameters
        } else if (key == "max-overall-mem") {
            file >> systemConfig.maxOverallMem;
        } else if (key == "mem-per-frame") {
            file >> systemConfig.memPerFrame;
        } else if (key == "mem-per-proc") {
            file >> systemConfig.memPerProc;

        } else {
            std::cerr << "Unknown config key: " << key << "\n";
        }
    }

    file.close();
    return true;
}
