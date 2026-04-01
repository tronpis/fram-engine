#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <string>
#include <memory>

namespace FarmEngine {

struct SpriteRenderData {
    VkBuffer vertexBuffer;
    VkBuffer indexBuffer;
    uint32_t vertexCount;
    uint32_t indexCount;
};

struct SpriteInstanceData {
    glm::vec4 position;    // x, y, width, height
    glm::vec4 color;       // r, g, b, a
    glm::vec4 uvRect;      // u, v, uWidth, vHeight
    float rotation;
    int32_t textureIndex;
};

class SpriteRenderer2D {
public:
    SpriteRenderer2D(VkDevice device, VkPhysicalDevice physicalDevice);
    ~SpriteRenderer2D();
    
    bool initialize(uint32_t maxSprites = 10000);
    void cleanup();
    
    // Rendering
    void beginFrame();
    void submitSprite(const SpriteInstanceData& data);
    void endFrame(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout);
    
    // Batch management
    void flush();
    void reset();
    
    // Texture binding
    void bindTexture(uint32_t slot, VkDescriptorSet descriptorSet);
    
    uint32_t getBatchCount() const { return batchCount; }
    uint32_t getMaxSprites() const { return maxSprites; }
    
private:
    void createBuffers();
    void createPipeline();
    void createDescriptorSets();
    void updateBuffers();
    
    VkDevice device;
    VkPhysicalDevice physicalDevice;
    
    // Buffers
    VkBuffer vertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory vertexBufferMemory = VK_NULL_HANDLE;
    VkBuffer indexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory indexBufferMemory = VK_NULL_HANDLE;
    VkBuffer instanceBuffer = VK_NULL_HANDLE;
    VkDeviceMemory instanceBufferMemory = VK_NULL_HANDLE;
    
    // Pipeline
    VkPipeline pipeline = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
    
    // Data
    std::vector<SpriteInstanceData> spriteData;
    uint32_t maxSprites;
    uint32_t batchCount;
    uint32_t currentIndex;
    
    bool initialized = false;
};

// Tilemap rendering
struct TileData {
    uint32_t tileID;
    uint32_t layer;
    uint8_t flags;  // Flip H, Flip V, Rotate
    uint8_t padding[3];
};

class TilemapRenderer2D {
public:
    TilemapRenderer2D(VkDevice device, VkPhysicalDevice physicalDevice);
    ~TilemapRenderer2D();
    
    bool initialize(uint32_t mapWidth, uint32_t mapHeight, uint32_t tileSize);
    void cleanup();
    
    // Map manipulation
    void setTile(uint32_t x, uint32_t y, uint32_t layer, const TileData& tile);
    void setTiles(const std::vector<TileData>& tiles);
    
    // Rendering
    void render(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout,
                const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix);
    
    // Culling
    void setVisibleRegion(int32_t startX, int32_t startY, 
                         int32_t endX, int32_t endY);
    
    uint32_t getWidth() const { return mapWidth; }
    uint32_t getHeight() const { return mapHeight; }
    uint32_t getLayerCount() const { return layerCount; }
    
private:
    void rebuildMesh();
    void createBuffers();
    
    VkDevice device;
    VkPhysicalDevice physicalDevice;
    
    uint32_t mapWidth;
    uint32_t mapHeight;
    uint32_t tileSize;
    uint32_t layerCount;
    
    std::vector<std::vector<TileData>> layers;
    
    // Buffers
    VkBuffer vertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory vertexBufferMemory = VK_NULL_HANDLE;
    VkBuffer indexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory indexBufferMemory = VK_NULL_HANDLE;
    
    // Visibility
    int32_t visibleStartX = 0;
    int32_t visibleStartY = 0;
    int32_t visibleEndX = -1;
    int32_t visibleEndY = -1;
    
    bool needsRebuild = true;
};

} // namespace FarmEngine
