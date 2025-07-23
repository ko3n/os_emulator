#pragma once
#include <vector>
#include <map>
#include <string>

struct MemBlock {
    int start;
    int end;
    int processId; // -1 for free
};

class MemoryManager {
public:
    MemoryManager(int totalMem, int perProc);
    bool allocate(int processId);
    void release(int processId);
    int getExternalFragmentationKB() const;
    void printMemoryAscii(std::ostream &out) const;
    int getProcessesInMemory() const;
private:
    int totalMemory;
    int memPerProc;
    std::vector<MemBlock> blocks;
};