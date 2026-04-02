#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <string>
#include <memory>
#include <unordered_map>
#include <functional>
#include <glm/glm.hpp>

namespace FarmEngine {

// ============================================================================
// RESOURCE SYSTEM - Memory Allocator (VMA-like)
// ============================================================================

enum class MemoryType {
    GPU_ONLY,           // VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
    CPU_TO_GPU,         // VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | HOST_COHERENT_BIT
    GPU_TO_CPU,         // VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | HOST_CACHED_BIT
    LAZY_ALLOCATED      // VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT
};

struct MemoryAllocation {
    VkDeviceMemory memory = VK_NULL_HANDLE;
    void* mappedData = nullptr;
    VkDeviceSize size = 0;
    VkDeviceSize offset = 0;
    uint32_t memoryTypeIndex = 0;
    
    bool isMapped() const { return mappedData != nullptr; }
};

class MemoryAllocator {
public:
    MemoryAllocator(VkPhysicalDevice physicalDevice, VkDevice device);
    ~MemoryAllocator();
    
    bool initialize();
    void cleanup();
    
    MemoryAllocation allocate(VkDeviceSize size, VkDeviceSize alignment,
                              MemoryType type, VkBuffer buffer = VK_NULL_HANDLE);
    void free(MemoryAllocation& allocation);
    
    void* map(MemoryAllocation& allocation);
    void unmap(MemoryAllocation& allocation);
    
    void bindBuffer(VkBuffer buffer, MemoryAllocation& allocation);
    void bindImage(VkImage image, MemoryAllocation& allocation);
    
    // Stats
    size_t getTotalAllocatedSize() const { return totalAllocatedSize; }
    size_t getAllocationCount() const { return allocationCount; }
    
private:
    uint32_t findMemoryType(uint32_t typeFilter, MemoryType memType);
    void createPools();
    
    VkPhysicalDevice physicalDevice;
    VkDevice device;
    VkPhysicalDeviceMemoryProperties memoryProperties;
    
    std::vector<VkDeviceMemory> deviceMemories;
    size_t totalAllocatedSize = 0;
    size_t allocationCount = 0;
    
    struct MemoryPool {
        VkDeviceMemory memory;
        VkDeviceSize size;
        VkDeviceSize used;
        uint32_t typeIndex;
    };
    std::vector<MemoryPool> pools[4]; // One per MemoryType
};

// ============================================================================
// RESOURCE SYSTEM - Buffer Pool
// ============================================================================

enum class BufferUsage {
    Vertex,
    Index,
    Uniform,
    Storage,
    Transfer,
    Dynamic
};

struct BufferConfig {
    BufferUsage usage;
    VkDeviceSize size;
    MemoryType memoryType = MemoryType::CPU_TO_GPU;
    bool dynamic = false;
};

class Buffer {
public:
    Buffer(VkDevice device, MemoryAllocator& allocator);
    ~Buffer();
    
    bool initialize(const BufferConfig& config);
    void cleanup();
    
    VkBuffer getHandle() const { return buffer; }
    VkDeviceSize getSize() const { return size; }
    void* map();
    void unmap();
    
    void update(const void* data, VkDeviceSize dataSize, VkDeviceSize offset = 0);
    
    const BufferConfig& getConfig() const { return config; }
    
private:
    VkDevice device;
    MemoryAllocator& allocator;
    
    VkBuffer buffer = VK_NULL_HANDLE;
    MemoryAllocation allocation;
    BufferConfig config;
    VkDeviceSize size = 0;
};

class BufferPool {
public:
    BufferPool(VkDevice device, MemoryAllocator& allocator);
    ~BufferPool();
    
    bool initialize(size_t initialVertexCount = 100000,
                    size_t initialIndexCount = 500000,
                    size_t initialUniformCount = 1000);
    void cleanup();
    
    // Allocate from pools
    Buffer* allocateVertex(VkDeviceSize size);
    Buffer* allocateIndex(VkDeviceSize size);
    Buffer* allocateUniform(VkDeviceSize size);
    Buffer* allocateStorage(VkDeviceSize size);
    
    // Dynamic buffers for streaming
    Buffer* createDynamic(BufferUsage usage, VkDeviceSize size);
    
    void resetFrame();
    
    size_t getVertexBufferSize() const { return vertexBuffers.size(); }
    size_t getIndexBufferSize() const { return indexBuffers.size(); }
    size_t getUniformBufferSize() const { return uniformBuffers.size(); }
    
private:
    Buffer* createBuffer(const BufferConfig& config);
    
    VkDevice device;
    MemoryAllocator& allocator;
    
    std::vector<std::unique_ptr<Buffer>> vertexBuffers;
    std::vector<std::unique_ptr<Buffer>> indexBuffers;
    std::vector<std::unique_ptr<Buffer>> uniformBuffers;
    std::vector<std::unique_ptr<Buffer>> storageBuffers;
    std::vector<std::unique_ptr<Buffer>> dynamicBuffers;
    
    // Ring buffer for dynamic allocations
    size_t dynamicOffset = 0;
};

// ============================================================================
// RESOURCE SYSTEM - Texture Pool
// ============================================================================

enum class TextureFormat {
    RGBA8,
    RGBA16F,
    RGBA32F,
    R8,
    R16F,
    R32F,
    Depth24,
    Depth32F,
    BC1,
    BC3,
    BC7
};

struct TextureConfig {
    uint32_t width;
    uint32_t height;
    uint32_t depth = 1;
    uint32_t mipLevels = 1;
    uint32_t arrayLayers = 1;
    TextureFormat format = TextureFormat::RGBA8;
    VkImageUsageFlags usage = VK_IMAGE_USAGE_SAMPLED_BIT;
    bool cubemap = false;
};

class Texture {
public:
    Texture(VkDevice device, VkPhysicalDevice physicalDevice, MemoryAllocator& allocator);
    ~Texture();
    
    bool initialize(const TextureConfig& config);
    bool loadFromFile(const std::string& path);
    bool loadData(const void* data, uint32_t width, uint32_t height, 
                  TextureFormat format = TextureFormat::RGBA8);
    void cleanup();
    
    VkImageView getView() const { return view; }
    VkSampler getSampler() const { return sampler; }
    VkImage getImage() const { return image; }
    
    uint32_t getWidth() const { return config.width; }
    uint32_t getHeight() const { return config.height; }
    uint32_t getMipLevels() const { return config.mipLevels; }
    
    void generateMipmaps(VkCommandBuffer commandBuffer);
    
private:
    void createImage();
    void createView();
    void createSampler();
    void transitionLayout(VkCommandBuffer commandBuffer, VkImageLayout oldLayout, 
                         VkImageLayout newLayout);
    void copyBufferToImage(VkCommandBuffer commandBuffer, VkBuffer buffer);
    
    VkDevice device;
    VkPhysicalDevice physicalDevice;
    MemoryAllocator& allocator;
    
    VkImage image = VK_NULL_HANDLE;
    MemoryAllocation allocation;
    VkImageView view = VK_NULL_HANDLE;
    VkSampler sampler = VK_NULL_HANDLE;
    
    TextureConfig config;
    VkFormat vkFormat;
};

class TexturePool {
public:
    TexturePool(VkDevice device, VkPhysicalDevice physicalDevice, MemoryAllocator& allocator);
    ~TexturePool();
    
    bool initialize(size_t maxTextures = 1024);
    void cleanup();
    
    Texture* createTexture(const TextureConfig& config);
    Texture* loadTexture(const std::string& path);
    Texture* getWhiteTexture();
    Texture* getBlackTexture();
    Texture* getNormalTexture();
    
    VkDescriptorSet getDescriptorSet(uint32_t textureIndex);
    
    size_t getTextureCount() const { return textures.size(); }
    
private:
    void createDefaultTextures();
    void createDescriptorPool();
    
    VkDevice device;
    VkPhysicalDevice physicalDevice;
    MemoryAllocator& allocator;
    
    std::vector<std::unique_ptr<Texture>> textures;
    Texture* whiteTexture = nullptr;
    Texture* blackTexture = nullptr;
    Texture* normalTexture = nullptr;
    
    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
    std::vector<VkDescriptorSet> descriptorSets;
    VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
};

// ============================================================================
// RESOURCE SYSTEM - Descriptor Allocator
// ============================================================================

enum class DescriptorType {
    UniformBuffer,
    StorageBuffer,
    SampledImage,
    Sampler,
    CombinedImageSampler,
    InputAttachment
};

struct DescriptorSetLayoutBinding {
    uint32_t binding;
    DescriptorType type;
    uint32_t count = 1;
    VkShaderStageFlags stages = VK_SHADER_STAGE_ALL;
    bool immutableSampler = false;
    VkSampler sampler = VK_NULL_HANDLE;
};

class DescriptorSetLayout {
public:
    DescriptorSetLayout(VkDevice device);
    ~DescriptorSetLayout();
    
    bool initialize(const std::vector<DescriptorSetLayoutBinding>& bindings);
    void cleanup();
    
    VkDescriptorSetLayout getHandle() const { return layout; }
    
private:
    VkDevice device;
    VkDescriptorSetLayout layout = VK_NULL_HANDLE;
};

class DescriptorAllocator {
public:
    DescriptorAllocator(VkDevice device);
    ~DescriptorAllocator();
    
    bool initialize(size_t maxSets = 1000, 
                   size_t maxUniformBuffers = 1000,
                   size_t maxSamplers = 1000,
                   size_t maxSampledImages = 2000);
    void cleanup();
    
    VkDescriptorSet allocate(DescriptorSetLayout* layout);
    void free(VkDescriptorSet set);
    void reset();
    
    void updateBuffer(VkDescriptorSet set, uint32_t binding, 
                     VkBuffer buffer, VkDeviceSize offset, VkDeviceSize range);
    void updateImage(VkDescriptorSet set, uint32_t binding,
                    VkImageView view, VkSampler sampler);
    
    size_t getUsedSets() const { return usedSets; }
    size_t getMaxSets() const { return maxSets; }
    
private:
    VkDevice device;
    
    VkDescriptorPool pool = VK_NULL_HANDLE;
    std::vector<VkDescriptorPool> additionalPools;
    
    size_t maxSets;
    size_t usedSets = 0;
    
    std::vector<VkDescriptorSet> freeSets;
};

// ============================================================================
// RESOURCE SYSTEM - Main Resource Manager
// ============================================================================

class ResourceSystem {
public:
    static ResourceSystem& getInstance();
    
    bool initialize(VkInstance instance, VkPhysicalDevice physicalDevice, 
                   VkDevice device, VkQueue graphicsQueue, VkQueue transferQueue);
    void cleanup();
    
    // Accessors
    MemoryAllocator& getMemoryAllocator() { return *memoryAllocator; }
    BufferPool& getBufferPool() { return *bufferPool; }
    TexturePool& getTexturePool() { return *texturePool; }
    DescriptorAllocator& getDescriptorAllocator() { return *descriptorAllocator; }
    
    // Command buffer for uploads
    VkCommandBuffer getUploadCommandBuffer();
    void flushUploads();
    
private:
    ResourceSystem() = default;
    ~ResourceSystem();
    
    void createUploadContext();
    
    VkInstance instance = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    VkQueue graphicsQueue = VK_NULL_HANDLE;
    VkQueue transferQueue = VK_NULL_HANDLE;
    
    std::unique_ptr<MemoryAllocator> memoryAllocator;
    std::unique_ptr<BufferPool> bufferPool;
    std::unique_ptr<TexturePool> texturePool;
    std::unique_ptr<DescriptorAllocator> descriptorAllocator;
    
    // Upload context
    VkCommandPool uploadCommandPool = VK_NULL_HANDLE;
    VkCommandBuffer uploadCommandBuffer = VK_NULL_HANDLE;
    VkFence uploadFence = VK_NULL_HANDLE;
};

} // namespace FarmEngine
