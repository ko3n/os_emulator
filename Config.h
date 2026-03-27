#ifndef CONFIG_H
#define CONFIG_H

#include <string>

struct Config {
    int numCPU;
    std::string scheduler;
    int quantumCycles;
    int batchProcessFreq;
    int minInstructions;
    int maxInstructions;
    int delayPerExec;
    // MO2 Parameters
    int maxOverallMem;
    int memPerFrame;
    int minMemPerProc;
    int maxMemPerProc;
};

extern Config systemConfig;

bool loadConfig(const std::string& filename);

#endif
