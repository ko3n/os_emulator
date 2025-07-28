#include "MemoryManager.h"
#include <vector>
#include <cstddef>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <ctime>
#include <algorithm>


// Implementation of MemoryManager methods
MemoryManager::MemoryManager(size_t total, size_t perProc) : totalSize(total), memPerProc(perProc) {
    blocks.push_back({0, totalSize, nullptr});
}

bool MemoryManager::allocate(Process* p) {
    for (auto it = blocks.begin(); it != blocks.end(); ++it) {
        if (it->owner == nullptr && (it->end - it->start) >= memPerProc) {
            size_t allocStart = it->end - memPerProc;
            size_t allocEnd = it->end;
            // Shrink free block
            if ((it->end - it->start) == memPerProc) {
                it->owner = p;
                p->memStart = it->start;
                p->memEnd = it->end;
            } else {
                // Split block
                MemoryBlock allocBlock = {allocStart, allocEnd, p};
                it->end = allocStart;
                blocks.insert(it + 1, allocBlock);
                p->memStart = allocStart;
                p->memEnd = allocEnd;
            }
            p->hasMemory = true;
            // Debug output: print block list after allocation
            // std::cout << "[MemoryManager] After allocating " << p->name << ":\n";
            // for (const auto& block : blocks) {
            //     std::cout << "  Block: [" << block.start << ", " << block.end << "] " << (block.owner ? block.owner->name : "FREE") << "\n";
            // }
            return true;
        }
    }
    p->hasMemory = false;
    // Debug output: allocation failed
    // std::cout << "[MemoryManager] Allocation failed for " << p->name << "\n";
    return false;
}

void MemoryManager::free(Process* p) {
    for (auto it = blocks.begin(); it != blocks.end(); ++it) {
        if (it->owner == p) {
            it->owner = nullptr;
            p->hasMemory = false;
            // Merge with previous free block
            if (it != blocks.begin()) {
                auto prev = it - 1;
                if (prev->owner == nullptr) {
                    prev->end = it->end;
                    it = blocks.erase(it);
                    --it;
                }
            }
            // Merge with next free block
            if ((it + 1) != blocks.end() && (it + 1)->owner == nullptr) {
                it->end = (it + 1)->end;
                blocks.erase(it + 1);
            }
            break;
        }
    }
}

size_t MemoryManager::getExternalFragmentation() const {
    size_t frag = 0;
    for (const auto& block : blocks) {
        if (block.owner == nullptr && (block.end - block.start) < memPerProc) {
            frag += (block.end - block.start);
        }
    }
    return frag;
}

int MemoryManager::getNumProcessesInMemory() const {
    int count = 0;
    for (const auto& block : blocks) {
        if (block.owner != nullptr) count++;
    }
    return count;
}

void MemoryManager::printMemoryMap(std::ostream& out) const {
    out << "----end---- = " << totalSize << "\n";

    // Print only blocks that are allocated, in descending address order
    for (auto it = blocks.rbegin(); it != blocks.rend(); ++it) {
        if (it->owner != nullptr) {
            out << it->end << "\n";
            out << it->owner->name << "\n";
            out << it->start << "\n";
        }
    }

    out << "----start---- = 0\n";
}

void outputMemorySnapshot(const MemoryManager& mm, int quantumCycle) {
    // Always use 4 files, wrap quantumCycle to 1-4
    int fileQuantum = ((quantumCycle - 1) % 4) + 1;
    std::ostringstream filename;
    filename << "memory_stamp_" << std::setw(2) << std::setfill('0') << fileQuantum << ".txt";

    std::ofstream outFile(filename.str(), std::ios::app);
    if (!outFile.is_open()) {
        std::string fullPath = "./" + filename.str();
        outFile.open(fullPath, std::ios::app);
        if (!outFile.is_open()) {
            return;
        }
    }

    // Get current timestamp (matching the format in attachment)
    std::time_t now = std::time(nullptr);
    char timeBuffer[100];
    std::strftime(timeBuffer, sizeof(timeBuffer), "%m/%d/%Y %I:%M:%S%p", std::localtime(&now));
    outFile << "Timestamp: (" << timeBuffer << ")\n";

    // Number of processes in memory
    int processCount = mm.getNumProcessesInMemory();
    outFile << "Number of processes in memory: " << processCount << "\n";

    // Total external fragmentation in KB
    outFile << "Total external fragmentation in KB: " << mm.getExternalFragmentation() / 1024 << "\n";

    // ASCII printout of memory (matching attachment format)
    outFile << "----end---- = " << mm.getTotalSize() << "\n";

    // Print only allocated blocks in descending address order (matching sample)
    for (auto it = mm.getBlocks().rbegin(); it != mm.getBlocks().rend(); ++it) {
        if (it->owner != nullptr) {
            outFile << it->end << "\n";
            outFile << it->owner->name << "\n";
            outFile << it->start << "\n";
        }
    }

    outFile << "----start---- = 0\n";
    outFile << "\n"; // Add separator between snapshots
    outFile.close();
}
