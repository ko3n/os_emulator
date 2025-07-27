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
            return true;
        }
    }
    p->hasMemory = false;
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
    std::ostringstream fname;
    fname << "memory_stamp_" << quantumCycle << ".txt";
    std::ofstream out(fname.str(), std::ios::app); // append mode
    if (!out.is_open()) return;
    // Timestamp
    std::time_t now = std::time(nullptr);
    out << "Timestamp: (" << std::put_time(std::localtime(&now), "%m/%d/%Y %I:%M:%S%p") << ")\n";
    out << "Number of processes in memory: " << mm.getNumProcessesInMemory() << "\n";
    out << "Total external fragmentation in KB: " << mm.getExternalFragmentation() / 1024 << "\n";
    mm.printMemoryMap(out);
    out << "\n";
    out.close();
}
