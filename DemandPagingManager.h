#pragma once
#include "Process.h"
#include <vector>
#include <unordered_map>
#include <queue>
#include <set>
#include <fstream>

// Simulate backing store for swapped pages
struct BackingStore {
    std::unordered_map<int, std::vector<char>> store; // key: process id + page num
    void savePage(int pid, int pageNum, const std::vector<char>& page) {
        store[(pid << 16) | pageNum] = page;
    }
    std::vector<char> loadPage(int pid, int pageNum) {
        return store[(pid << 16) | pageNum];
    }
};

struct PageTableEntry {
    int frameNumber;
    bool present;
    bool dirty;
    bool referenced;
};

class DemandPagingManager {
public:
    DemandPagingManager(size_t numFrames, size_t pageSize);
    ~DemandPagingManager() = default;

    // Called when process accesses a virtual address
    bool accessPage(Process* proc, size_t virtualAddr, bool write);

    // Called when process is created
    void allocateProcess(Process* proc, int memRequired);

    // On process exit
    void freeProcess(Process* proc);

    void printState(std::ostream& out);

private:
    size_t numFrames;
    size_t pageSize;
    std::vector<int> frameTable; // -1 if free, else process id
    std::vector<std::pair<int, int>> frameUsage; // (process id, page num)
    std::unordered_map<int, std::unordered_map<int, PageTableEntry>> pageTables; // proc id -> (page# -> entry)
    BackingStore backingStore;

    // Page replacement queue (FIFO)
    std::queue<std::pair<int, int>> fifoQueue;

    // Page fault handler
    void handlePageFault(int pid, int pageNum);

    int findFreeFrame();
    int evictPage();
};