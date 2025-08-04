
#pragma once
#include "Process.h"
#include <vector>
#include <cstddef>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <ctime>
#include <algorithm>

struct MemoryBlock {
    size_t start;
    size_t end;
    Process* owner; // nullptr if free
};

class MemoryManager {
    std::vector<MemoryBlock> blocks;
    size_t totalSize;
    size_t memPerFrame;
public:
    MemoryManager(size_t total, size_t frameSize);
    bool allocate(Process* p);
    void free(Process* p);
    size_t getExternalFragmentation() const;
    int getNumProcessesInMemory() const;
    void printMemoryMap(std::ostream& out) const;
    size_t getTotalSize() const { return totalSize; }
    const std::vector<MemoryBlock>& getBlocks() const { return blocks; }
};

void outputMemorySnapshot(const MemoryManager& mm, int quantumCycle);
