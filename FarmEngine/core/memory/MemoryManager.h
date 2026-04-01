#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>
#include <memory>

namespace FarmEngine {

// Estados de memoria para debugging
enum class MemoryTag {
    None = 0,
    Engine,
    Renderer,
    Physics,
    Audio,
    World,
    Simulation,
    UI,
    Assets,
    Temporary
};

class MemoryManager {
public:
    static MemoryManager& getInstance();
    
    void* allocate(size_t size, MemoryTag tag = MemoryTag::None);
    void deallocate(void* ptr);
    
    void* reallocate(void* ptr, size_t newSize);
    
    // Estadísticas de memoria
    size_t getTotalAllocated() const;
    size_t getAllocationsByTag(MemoryTag tag) const;
    size_t getAllocationCount() const;
    
    // Limpieza y debugging
    void reset();
    void dumpLeaks();
    
private:
    MemoryManager() = default;
    ~MemoryManager();
    
    MemoryManager(const MemoryManager&) = delete;
    MemoryManager& operator=(const MemoryManager&) = delete;
    
    struct AllocationHeader {
        void* ptr;
        size_t size;
        MemoryTag tag;
        uint32_t id;
    };
    
    std::vector<AllocationHeader> allocations;
    size_t totalAllocated = 0;
    uint32_t allocationCounter = 0;
};

// Macros para tracking de memoria
#ifdef FE_DEBUG
    #define FE_ALLOC(size) MemoryManager::getInstance().allocate(size, MemoryTag::Temporary)
    #define FE_FREE(ptr) MemoryManager::getInstance().deallocate(ptr)
#else
    #define FE_ALLOC(size) malloc(size)
    #define FE_FREE(ptr) free(ptr)
#endif

// Allocators especializados
template<typename T>
class PoolAllocator {
public:
    PoolAllocator(size_t poolSize = 1024);
    ~PoolAllocator();
    
    T* allocate();
    void deallocate(T* ptr);
    
    size_t getAvailableBlocks() const;
    
private:
    std::vector<T*> freeList;
    std::vector<uint8_t*> pools;
    size_t blockSize;
    size_t poolSize;
};

template<typename T>
class StackAllocator {
public:
    StackAllocator(size_t size);
    ~StackAllocator();
    
    void* allocate(size_t alignment, size_t size);
    void reset();
    
    size_t getUsedMemory() const;
    
private:
    uint8_t* buffer;
    size_t totalSize;
    size_t usedOffset;
};

} // namespace FarmEngine
