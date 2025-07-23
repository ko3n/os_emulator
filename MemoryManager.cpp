#include "MemoryManager.h"
#include <iomanip>
#include <ctime>
#include <algorithm>
MemoryManager::MemoryManager(int totalMem, int perProc)
    : totalMemory(totalMem), memPerProc(perProc) {
    blocks.push_back({0, totalMemory, -1});
}

bool MemoryManager::allocate(int processId) {
    for (auto it = blocks.begin(); it != blocks.end(); ++it) {
        if (it->processId == -1 && (it->end - it->start) >= memPerProc) {
            int start = it->start;
            int end = start + memPerProc;
            if (end < it->end) {
                blocks.insert(it + 1, {end, it->end, -1});
            }
            *it = {start, end, processId};
            return true;
        }
    }
    return false;
}

void MemoryManager::release(int processId) {
    for (auto it = blocks.begin(); it != blocks.end(); ++it) {
        if (it->processId == processId) {
            it->processId = -1;
            // Merge with previous
            if (it != blocks.begin()) {
                auto prev = it - 1;
                if (prev->processId == -1) {
                    prev->end = it->end;
                    blocks.erase(it);
                    it = prev;
                }
            }
            // Merge with next
            if (it + 1 != blocks.end()) {
                auto next = it + 1;
                if (next->processId == -1) {
                    it->end = next->end;
                    blocks.erase(next);
                }
            }
            break;
        }
    }
}

int MemoryManager::getExternalFragmentationKB() const {
    int frag = 0;
    for (const auto& b : blocks) {
        if (b.processId == -1 && (b.end - b.start) < memPerProc) {
            frag += (b.end - b.start);
        }
    }
    return frag / 1024;
}

void MemoryManager::printMemoryAscii(std::ostream &out) const {
    out << "----end---- = " << totalMemory << "\n";
    for (auto it = blocks.rbegin(); it != blocks.rend(); ++it) {
        out << it->end << "\n";
        if (it->processId != -1) {
            out << "P" << it->processId << "\n";
        }
        out << it->start << "\n";
    }
    out << "----start---- = 0\n";
}

int MemoryManager::getProcessesInMemory() const {
    int count = 0;
    for (const auto& b : blocks) {
        if (b.processId != -1) count++;
    }
    return count;
}