#include "MemoryManager.h"
#include "Process.h"
#include "Scheduler.h"
#include <algorithm>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <set>
#include <functional>
#include <filesystem>
#include <sstream>

const std::string BACKING_STORE_FILENAME = "csopesy-backing-store.txt";

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

    // Initialize readable backing store file at startup
    initializeBackingStore();
}

MemoryManager::~MemoryManager() {
    // Clean up
}

bool MemoryManager::allocateProcess(Process* process) {
    if (!process) return false;

    // Calculate number of pages needed
    int pagesNeeded = (process->memRequired + frameSize - 1) / frameSize;
    
    // Check if we have enough free frames 
    pageTables[process].resize(pagesNeeded);

    for (int i = 0; i < pagesNeeded; i++) {
        pageTables[process][i].frameNumber = -1;
        pageTables[process][i].isValid = false;
        pageTables[process][i].isDirty = false;
        pageTables[process][i].isReferenced = false;
    }

    process->hasMemory = true;
    return true;

    // Create page table for process 
    pageTables[process].resize(pagesNeeded);

    // Initialize page table entries as invalid
    for (int i = 0; i < pagesNeeded; i++) {
        pageTables[process][i].frameNumber = -1;
        pageTables[process][i].isValid = false;  // Pages start NOT in memory
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

// --- Backing store functions for readable text file ---

void MemoryManager::initializeBackingStore() {
    std::filesystem::path path(BACKING_STORE_FILENAME);
    int totalPages = totalMemory / frameSize;
    int totalBytes = totalPages * frameSize;
    bool needInit = false;
    if (!std::filesystem::exists(path)) {
        needInit = true;
    } else {
        std::ifstream ifs(BACKING_STORE_FILENAME, std::ios::binary | std::ios::ate);
        if (ifs.tellg() < totalBytes) {
            needInit = true;
        }
        ifs.close();
    }
    if (needInit) {
        std::ofstream ofs(BACKING_STORE_FILENAME, std::ios::binary | std::ios::trunc);
        std::vector<char> zeros(totalBytes, 0);
        ofs.write(zeros.data(), zeros.size());
        ofs.close();
    }
}

// Write the frame's bytes as "HEXx" or "NUL" for zero
void MemoryManager::writePageToBackingStore(int pageNumber, const std::vector<char>& data) {
    std::fstream fs(BACKING_STORE_FILENAME, std::ios::in | std::ios::out | std::ios::binary);
    if (!fs.is_open()) return;
    int offset = pageNumber * frameSize;
    fs.seekp(offset, std::ios::beg);
    fs.write(data.data(), frameSize);
    fs.close();
}

// Read a page from the backing store file, interpreting "NUL" or "HEXx"
std::vector<char> MemoryManager::readPageFromBackingStore(int pageIndex, int pageSize) {
    std::ifstream ifs(BACKING_STORE_FILENAME, std::ios::binary);
    std::vector<char> buffer(pageSize, 0);
    if (!ifs.is_open()) return buffer;
    int offset = pageIndex * pageSize;
    ifs.seekg(offset, std::ios::beg);
    ifs.read(buffer.data(), pageSize);
    ifs.close();
    return buffer;
}

// --- Integrate with existing eviction and load functions ---

void MemoryManager::evictPageToBackingStore(int frameNumber) {
    if (!frames[frameNumber].isOccupied) return;

    Process* process = frames[frameNumber].owner;
    int pageNumber = frames[frameNumber].virtualPageNumber;

    // Save frame's memory to readable backing store file
    int startAddr = frameNumber * frameSize;
    std::vector<char> pageData(physicalMemory.begin() + startAddr, physicalMemory.begin() + startAddr + frameSize);
    writePageToBackingStore(pageNumber, pageData);

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

void MemoryManager::loadPageFromBackingStore(Process* process, int pageNumber, int frameNumber) {
    // Load from readable text backing store
    std::vector<char> pageData = readPageFromBackingStore(pageNumber, frameSize);
    bool isEmpty = true;
    for (char c : pageData) {
        if (c != 0) { isEmpty = false; break; }
    }
    int startAddr = frameNumber * frameSize;
    if (!isEmpty) {
        for (int i = 0; i < frameSize; i++) {
            physicalMemory[startAddr + i] = pageData[i];
        }
    } else {
        // If not present, initialize simulated data as before
        int processHash = std::hash<std::string>{}(process->name) % 256;
        for (int i = 0; i < frameSize; i++) {
            physicalMemory[startAddr + i] = (processHash + pageNumber + i) % 256;
        }
    }
}

// --- All other code unchanged (getFreeFrames, printMemoryMap, etc.) ---
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