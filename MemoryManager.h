#ifndef MEMORYMANAGER_H
#define MEMORYMANAGER_H

#include <vector>
#include <map>
#include <queue>
#include <chrono>
#include <iostream>
#include <set>
#include <functional> 

// Forward declaration
class Process;

struct MemoryBlock {
    size_t start;
    size_t end;
    Process* owner; // nullptr if free
};

struct Frame {
    int frameId;
    Process* owner;
    int virtualPageNumber;
    bool isOccupied;
    bool isDirty;
};

struct PageTableEntry {
    int frameNumber;
    bool isValid;
    bool isDirty;
    bool isReferenced;
};

class MemoryManager {
private:
    std::vector<Frame> frames;
    std::map<Process*, std::vector<PageTableEntry>> pageTables;
    std::vector<char> physicalMemory; // Simulate actual memory
    
    int totalFrames;
    int frameSize;
    int totalMemory;
    
    // FIFO page replacement
    int fifoPointer;
    
public:
    MemoryManager(int totalMem, int frameSz); 
    ~MemoryManager();
    
    // Core demand paging functions
    bool allocateProcess(Process* process);
    void deallocateProcess(Process* process);
    bool handlePageFault(Process* process, int virtualPageNumber);
    bool translateAddress(Process* process, int virtualAddress, int& physicalAddress);
    
    // Frame management
    int findFreeFrame();
    int selectVictimFrame(); // FIFO page replacement
    void loadPageFromBackingStore(Process* process, int pageNumber, int frameNumber);
    void evictPageToBackingStore(int frameNumber);
    
    // Memory info functions
    int getTotalFrames() const;
    int getFreeFrames() const;
    int getUsedFrames() const;
    size_t getExternalFragmentation() const;
    int getNumProcessesInMemory() const;
    void printMemoryMap(std::ostream& out) const;
    
    // Process memory simulation
    void accessMemory(Process* process, int virtualAddress);
    
    // Legacy interface for compatibility
    bool allocate(Process* p) { return allocateProcess(p); }
    void free(Process* p) { deallocateProcess(p); }
};

void outputMemorySnapshot(const MemoryManager& mm, int quantumCycle);

#endif // MEMORYMANAGER_H
