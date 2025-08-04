#ifndef MEMORYMANAGER_H
#define MEMORYMANAGER_H

#include <vector>
#include <map>
#include <queue>
#include <chrono>
#include <iostream>
#include <set>
#include <functional>

// Forward declarations
class Process;
class Scheduler;  // Add this forward declaration

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
    std::vector<char> physicalMemory;
    
    int totalFrames;
    int frameSize;
    int totalMemory;
    int fifoPointer;
    Scheduler* scheduler;  // Change to pointer
    
public:
    MemoryManager(int totalMem, int frameSz);
    ~MemoryManager();
    
    // Add method to set scheduler reference
    void setScheduler(Scheduler* sched) { scheduler = sched; }
    
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

    uint16_t readWord(int physAddr) const {
        return *reinterpret_cast<const uint16_t*>(&physicalMemory[physAddr]);
    }
    void writeWord(int physAddr, uint16_t value) {
        *reinterpret_cast<uint16_t*>(&physicalMemory[physAddr]) = value;
    }
};

void outputMemorySnapshot(const MemoryManager& mm, int quantumCycle);

#endif // MEMORYMANAGER_H
