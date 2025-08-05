#include "MemoryManager.h"
#include "Process.h"
#include "Scheduler.h"  // Add this include
#include <algorithm>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <set>
#include <functional>  

MemoryManager::MemoryManager(int totalMem, int frameSz) 
    : totalMemory(totalMem), frameSize(frameSz), fifoPointer(0), scheduler(nullptr) {
    
    totalFrames = totalMemory / frameSize;
    frames.resize(totalFrames);
    physicalMemory.resize(totalMemory, 0);
    
    // Initialize frames
    for (int i = 0; i < totalFrames; i++) {
        frames[i].frameId = i;
        frames[i].owner = nullptr;
        frames[i].virtualPageNumber = -1;
        frames[i].isOccupied = false;
        frames[i].isDirty = false;
    }
}

MemoryManager::~MemoryManager() {
    // Clean up
}

bool MemoryManager::allocateProcess(Process* process) {
    if (!process) return false;
    
    // Calculate number of pages needed
    int pagesNeeded = (process->memRequired + frameSize - 1) / frameSize;
    
    // Create page table for process 
    pageTables[process].resize(pagesNeeded);
    
    // Initialize page table entries as invalid (not in memory)
    for (int i = 0; i < pagesNeeded; i++) {
        pageTables[process][i].frameNumber = -1;
        pageTables[process][i].isValid = false;
        pageTables[process][i].isDirty = false;
        pageTables[process][i].isReferenced = false;
    }
    
    process->hasMemory = true;
    return true;
}

void MemoryManager::deallocateProcess(Process* process) {
    if (!process || pageTables.find(process) == pageTables.end()) {
        return;
    }
    
    // Free all frames used by this process
    for (int i = 0; i < totalFrames; i++) {
        if (frames[i].owner == process) {
            frames[i].owner = nullptr;
            frames[i].virtualPageNumber = -1;
            frames[i].isOccupied = false;
            frames[i].isDirty = false;
        }
    }
    
    // Remove page table
    pageTables.erase(process);
    process->hasMemory = false;
}

bool MemoryManager::handlePageFault(Process* process, int virtualPageNumber) {
    if (pageTables.find(process) == pageTables.end()) {
        return false;
    }
    
    if (virtualPageNumber >= pageTables[process].size()) {
        return false; // Invalid page number
    }
    
    // Find free frame or select victim
    int frameNumber = findFreeFrame();
    if (frameNumber == -1) {
        // No free frames, need to evict a page
        frameNumber = selectVictimFrame();
        evictPageToBackingStore(frameNumber);
        
        // Increment pages out counter
        if (scheduler) {
            scheduler->incrementPagedOut();
        }
    }
    
    // Load page from backing store
    loadPageFromBackingStore(process, virtualPageNumber, frameNumber);
    
    // Increment pages in counter
    if (scheduler) {
        scheduler->incrementPagedIn();
    }
    
    // Update page table
    pageTables[process][virtualPageNumber].frameNumber = frameNumber;
    pageTables[process][virtualPageNumber].isValid = true;
    pageTables[process][virtualPageNumber].isReferenced = true;
    
    // Update frame info
    frames[frameNumber].owner = process;
    frames[frameNumber].virtualPageNumber = virtualPageNumber;
    frames[frameNumber].isOccupied = true;
    
    // std::cout << "[PAGE LOADED] Page " << virtualPageNumber << " of " << process->name 
    //           << " loaded into frame " << frameNumber << std::endl;
    
    // After loading the page and updating the page table:
    for (auto& entry : pageTables) {
        if (entry.first != process) {
            bool anyValid = false;
            for (const auto& page : entry.second) {
                if (page.isValid) {
                    anyValid = true;
                    break;
                }
            }
            if (!anyValid) {
                entry.first->hasMemory = false;
            }
        }
    }
    process->hasMemory = true;
    
    return true;
}

bool MemoryManager::translateAddress(Process* process, int virtualAddress, int& physicalAddress) {
    if (pageTables.find(process) == pageTables.end()) {
        return false;
    }
    
    int pageNumber = virtualAddress / frameSize;
    int offset = virtualAddress % frameSize;
    
    if (pageNumber >= pageTables[process].size()) {
        return false; // Invalid address
    }
    
    // Check if page is in memory
    if (!pageTables[process][pageNumber].isValid) {
        // Page fault - load page
        if (!handlePageFault(process, pageNumber)) {
            return false;
        }
    }
    
    // Update reference bit
    pageTables[process][pageNumber].isReferenced = true;
    
    int frameNumber = pageTables[process][pageNumber].frameNumber;
    physicalAddress = frameNumber * frameSize + offset;
    return true;
}

int MemoryManager::findFreeFrame() {
    for (int i = 0; i < totalFrames; i++) {
        if (!frames[i].isOccupied) {
            return i;
        }
    }
    return -1; // No free frames
}

int MemoryManager::selectVictimFrame() {
    // FIFO: find next occupied frame
    int startPointer = fifoPointer;
    
    do {
        if (frames[fifoPointer].isOccupied) {
            int victim = fifoPointer;
            fifoPointer = (fifoPointer + 1) % totalFrames;
            return victim;
        }
        fifoPointer = (fifoPointer + 1) % totalFrames;
    } while (fifoPointer != startPointer);
    
    return 0; // Fallback
}

void MemoryManager::loadPageFromBackingStore(Process* process, int pageNumber, int frameNumber) {
    // Simulate loading from disk
    // std::cout << "[DISK I/O] Loading page " << pageNumber 
    //           << " of " << process->name << " into frame " << frameNumber << std::endl;
    
    // Simulate some data in the page
    int startAddr = frameNumber * frameSize;
    int processHash = std::hash<std::string>{}(process->name) % 256;
    for (int i = 0; i < frameSize; i++) {
        physicalMemory[startAddr + i] = (processHash + pageNumber + i) % 256;
    }
}

void MemoryManager::evictPageToBackingStore(int frameNumber) {
    if (!frames[frameNumber].isOccupied) return;

    Process* process = frames[frameNumber].owner;
    int pageNumber = frames[frameNumber].virtualPageNumber;

    // Mark page as invalid in page table
    if (pageTables.find(process) != pageTables.end() &&
        pageNumber < pageTables[process].size()) {
        pageTables[process][pageNumber].isValid = false;
        pageTables[process][pageNumber].frameNumber = -1;
    }

    // Check if the process has any valid pages left in memory
    bool anyValid = false;
    for (const auto& entry : pageTables[process]) {
        if (entry.isValid) {
            anyValid = true;
            break;
        }
    }
    if (!anyValid) {
        process->hasMemory = false;
    }
}

int MemoryManager::getFreeFrames() const {
    int count = 0;
    for (const auto& frame : frames) {
        if (!frame.isOccupied) count++;
    }
    return count;
}

int MemoryManager::getUsedFrames() const {
    return totalFrames - getFreeFrames();
}

void MemoryManager::accessMemory(Process* process, int virtualAddress) {
    int physicalAddress;
    if (translateAddress(process, virtualAddress, physicalAddress)) {
        // Memory access successful
        // std::cout << "[MEMORY ACCESS] " << process->name 
        //           << " accessed virtual addr " << virtualAddress 
        //           << " -> physical addr " << physicalAddress << std::endl;
    } else {
        std::cout << "[MEMORY ERROR] Failed to access address " 
                  << virtualAddress << " for " << process->name << std::endl;
    }
}

void MemoryManager::printMemoryMap(std::ostream& out) const {
    out << "====== MEMORY MAP ======\n";
    out << "Total Frames: " << totalFrames << "\n";
    out << "Free Frames: " << getFreeFrames() << "\n";
    out << "Used Frames: " << getUsedFrames() << "\n\n";
    
    for (int i = totalFrames - 1; i >= 0; i--) {
        out << "Frame " << std::setw(2) << i << ": ";
        if (frames[i].isOccupied) {
            out << frames[i].owner->name << " (Page " << frames[i].virtualPageNumber << ")";
        } else {
            out << "FREE";
        }
        out << "\n";
    }
    out << "========================\n";
}

size_t MemoryManager::getExternalFragmentation() const {
    return getFreeFrames() * frameSize;
}

int MemoryManager::getNumProcessesInMemory() const {
    std::set<Process*> uniqueProcesses;
    for (const auto& frame : frames) {
        if (frame.isOccupied) {
            uniqueProcesses.insert(frame.owner);
        }
    }
    return uniqueProcesses.size();
}

int MemoryManager::getTotalFrames() const {
    return totalFrames;
}
