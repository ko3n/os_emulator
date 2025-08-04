#include "DemandPagingManager.h"
#include <iostream>
#include <cstring>

DemandPagingManager::DemandPagingManager(size_t frames, size_t pSize)
    : numFrames(frames), pageSize(pSize), frameTable(frames, -1) {}

void DemandPagingManager::allocateProcess(Process* proc, int memRequired) {
    int pid = proc->id;
    int numPages = (memRequired + pageSize - 1) / pageSize;
    for (int p = 0; p < numPages; ++p) {
        pageTables[pid][p] = { -1, false, false, false };
    }
}

void DemandPagingManager::freeProcess(Process* proc) {
    int pid = proc->id;
    for (auto& kv : pageTables[pid]) {
        if (kv.second.present && kv.second.frameNumber >= 0) {
            frameTable[kv.second.frameNumber] = -1;
        }
    }
    pageTables.erase(pid);
}

bool DemandPagingManager::accessPage(Process* proc, size_t virtualAddr, bool write) {
    int pid = proc->id;
    int pageNum = virtualAddr / pageSize;
    auto& ptEntry = pageTables[pid][pageNum];
    ptEntry.referenced = true;

    if (!ptEntry.present) {
        // Page fault!
        handlePageFault(pid, pageNum);
        ptEntry = pageTables[pid][pageNum]; // update after fault
    }
    if (write) ptEntry.dirty = true;
    return true;
}

void DemandPagingManager::handlePageFault(int pid, int pageNum) {
    int frameNum = findFreeFrame();
    if (frameNum == -1) {
        frameNum = evictPage();
    }
    // Simulate loading page from backing store
    frameTable[frameNum] = pid;
    frameUsage[frameNum] = { pid, pageNum };
    pageTables[pid][pageNum].frameNumber = frameNum;
    pageTables[pid][pageNum].present = true;
    // FIFO
    fifoQueue.push({ pid, pageNum });
}

int DemandPagingManager::findFreeFrame() {
    for (int i = 0; i < numFrames; ++i) {
        if (frameTable[i] == -1) return i;
    }
    return -1;
}

int DemandPagingManager::evictPage() {
    // FIFO: evict oldest
    if (fifoQueue.empty()) throw std::runtime_error("No pages to evict");
    auto victim = fifoQueue.front(); fifoQueue.pop();
    int victimPid = victim.first;
    int victimPage = victim.second;
    int frameNum = pageTables[victimPid][victimPage].frameNumber;
    // Save to backing store if dirty
    if (pageTables[victimPid][victimPage].dirty) {
        std::vector<char> dummyPage(pageSize, 0); // Simulate page contents
        backingStore.savePage(victimPid, victimPage, dummyPage);
    }
    pageTables[victimPid][victimPage].present = false;
    pageTables[victimPid][victimPage].frameNumber = -1;
    frameTable[frameNum] = -1;
    return frameNum;
}

void DemandPagingManager::printState(std::ostream& out) {
    out << "=== Demand Paging Manager ===\n";
    out << "Frames: " << numFrames << " Page size: " << pageSize << "\n";
    for (const auto& pt : pageTables) {
        out << "Process " << pt.first << " page table:\n";
        for (const auto& kv : pt.second) {
            out << "  Page " << kv.first
                << " -> Frame " << kv.second.frameNumber
                << (kv.second.present ? " [RES]" : " [SWAP]") << "\n";
        }
    }
}