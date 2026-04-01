#include "MemoryManager.h"
#include <iostream>
#include <algorithm>

namespace FarmEngine {

MemoryManager& MemoryManager::getInstance() {
    static MemoryManager instance;
    return instance;
}

MemoryManager::~MemoryManager() {
    dumpLeaks();
}

void* MemoryManager::allocate(size_t size, MemoryTag tag) {
    void* ptr = malloc(size);
    
    if (ptr) {
        AllocationHeader header{ptr, size, tag, allocationCounter++};
        allocations.push_back(header);
        totalAllocated += size;
    }
    
    return ptr;
}

void MemoryManager::deallocate(void* ptr) {
    if (!ptr) return;
    
    // Buscar y remover la asignación por puntero
    auto it = std::find_if(allocations.begin(), allocations.end(),
        [ptr](const AllocationHeader& h) {
            return h.ptr == ptr;
        });
    
    if (it != allocations.end()) {
        totalAllocated -= it->size;
        allocations.erase(it);
    }
    
    free(ptr);
}

void* MemoryManager::reallocate(void* ptr, size_t newSize) {
    if (!ptr) return allocate(newSize);
    if (newSize == 0) {
        deallocate(ptr);
        return nullptr;
    }
    
    // Find old allocation to get old size
    size_t oldSize = 0;
    auto it = std::find_if(allocations.begin(), allocations.end(),
        [ptr](const AllocationHeader& h) {
            return h.ptr == ptr;
        });
    
    if (it != allocations.end()) {
        oldSize = it->size;
    }
    
    void* newPtr = realloc(ptr, newSize);
    if (newPtr) {
        if (it != allocations.end()) {
            // Update existing record
            totalAllocated -= it->size;
            it->ptr = newPtr;
            it->size = newSize;
            totalAllocated += newSize;
        } else {
            // Create new record if old one wasn't found
            AllocationHeader header{newPtr, newSize, MemoryTag::None, allocationCounter++};
            allocations.push_back(header);
            totalAllocated += newSize;
        }
    }
    
    return newPtr;
}

size_t MemoryManager::getTotalAllocated() const {
    return totalAllocated;
}

size_t MemoryManager::getAllocationsByTag(MemoryTag tag) const {
    size_t count = 0;
    for (const auto& alloc : allocations) {
        if (alloc.tag == tag) {
            count += alloc.size;
        }
    }
    return count;
}

size_t MemoryManager::getAllocationCount() const {
    return allocations.size();
}

void MemoryManager::reset() {
    for (auto& alloc : allocations) {
        // Liberar memoria asociada (simplificado)
    }
    allocations.clear();
    totalAllocated = 0;
    allocationCounter = 0;
}

void MemoryManager::dumpLeaks() {
    if (!allocations.empty()) {
        std::cout << "[MEMORY] Possible leaks detected: " << allocations.size() 
                  << " allocations, " << totalAllocated << " bytes\n";
    }
}

// PoolAllocator implementation
template<typename T>
PoolAllocator<T>::PoolAllocator(size_t poolSize) 
    : blockSize(sizeof(T)), poolSize(poolSize) {
    uint8_t* pool = new uint8_t[blockSize * poolSize];
    pools.push_back(pool);
    
    for (size_t i = 0; i < poolSize; ++i) {
        freeList.push_back(reinterpret_cast<T*>(pool + i * blockSize));
    }
}

template<typename T>
PoolAllocator<T>::~PoolAllocator() {
    for (auto* pool : pools) {
        delete[] pool;
    }
}

template<typename T>
T* PoolAllocator<T>::allocate() {
    if (freeList.empty()) {
        // Crear nuevo pool
        uint8_t* pool = new uint8_t[blockSize * poolSize];
        pools.push_back(pool);
        
        for (size_t i = 0; i < poolSize; ++i) {
            freeList.push_back(reinterpret_cast<T*>(pool + i * blockSize));
        }
    }
    
    T* ptr = freeList.back();
    freeList.pop_back();
    return ptr;
}

template<typename T>
void PoolAllocator<T>::deallocate(T* ptr) {
    freeList.push_back(ptr);
}

template<typename T>
size_t PoolAllocator<T>::getAvailableBlocks() const {
    return freeList.size();
}

// StackAllocator implementation
template<typename T>
StackAllocator<T>::StackAllocator(size_t size) 
    : totalSize(size), usedOffset(0) {
    buffer = new uint8_t[size];
}

template<typename T>
StackAllocator<T>::~StackAllocator() {
    delete[] buffer;
}

template<typename T>
void* StackAllocator<T>::allocate(size_t alignment, size_t size) {
    size_t alignedOffset = (usedOffset + alignment - 1) & ~(alignment - 1);
    
    if (alignedOffset + size > totalSize) {
        return nullptr; // Out of memory
    }
    
    void* ptr = buffer + alignedOffset;
    usedOffset = alignedOffset + size;
    return ptr;
}

template<typename T>
void StackAllocator<T>::reset() {
    usedOffset = 0;
}

template<typename T>
size_t StackAllocator<T>::getUsedMemory() const {
    return usedOffset;
}

// Explicit instantiations para tipos comunes
template class PoolAllocator<char>;
template class PoolAllocator<int>;
template class PoolAllocator<float>;
template class StackAllocator<char>;
template class StackAllocator<int>;
template class StackAllocator<float>;

} // namespace FarmEngine
