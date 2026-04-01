#pragma once

#include <vulkan/vulkan.h>
#include <memory>
#include <string>
#include <glm/glm.hpp>

namespace FarmEngine {

// Forward declarations
class Window;
class VulkanContext;
class ResourceSystem;
class PipelineSystem;
class RenderGraph;
class SpriteRenderer2D;
class TilemapRenderer2D;
class Renderer3D;
class ParticleSystem;
class InstancedRenderer;
class SkinnedMeshRenderer;

struct RendererConfig {
    uint32_t screenWidth = 1920;
    uint32_t screenHeight = 1080;
    bool vsync = true;
    bool fullscreen = false;
    bool debugMode = true;
    
    // Quality settings
    enum class Quality { Low, Medium, High, Ultra };
    Quality quality = Quality::High;
    
    // Features
    bool enableShadows = true;
    bool enableSSAO = true;
    bool enableBloom = true;
    bool enableMotionBlur = false;
    bool enableVolumetricFog = false;
    
    // LOD settings
    float lodBias = 0.0f;
    float drawDistance = 500.0f;
    
    // Vegetation
    uint32_t maxVegetationInstances = 100000;
    bool enableVegetationWind = true;
    
    // Particles
    uint32_t maxParticles = 50000;
};

struct FrameStats {
    uint32_t frameNumber = 0;
    float fps = 0.0f;
    float frameTime = 0.0f;
    
    uint32_t drawCalls = 0;
    uint32_t triangleCount = 0;
    uint32_t instanceCount = 0;
    uint32_t particleCount = 0;
    
    size_t gpuMemoryUsed = 0;
    size_t textureMemoryUsed = 0;
};

class Renderer {
public:
    static Renderer& getInstance();
    
    // Initialization
    bool initialize(void* windowHandle, void* windowInstance, 
                   const RendererConfig& config = {});
    void shutdown();
    
    // Frame management
    bool beginFrame();
    void endFrame();
    void present();
    
    // Resize
    void onWindowResize(uint32_t newWidth, uint32_t newHeight);
    
    // Access to subsystems
    VulkanContext& getVulkanContext() { return *vulkanContext; }
    ResourceSystem& getResourceSystem() { return *resourceSystem; }
    PipelineSystem& getPipelineSystem() { return *pipelineSystem; }
    RenderGraph& getRenderGraph() { return *renderGraph; }
    
    // 2D Rendering
    SpriteRenderer2D* getSpriteRenderer() { return spriteRenderer.get(); }
    TilemapRenderer2D* getTilemapRenderer() { return tilemapRenderer.get(); }
    
    // 3D Rendering
    Renderer3D* getRenderer3D() { return renderer3D.get(); }
    InstancedRenderer* getInstancedRenderer() { return instancedRenderer.get(); }
    SkinnedMeshRenderer* getSkinnedMeshRenderer() { return skinnedMeshRenderer.get(); }
    
    // Effects
    ParticleSystem* getParticleSystem() { return particleSystem.get(); }
    
    // Camera
    void setViewMatrix(const glm::mat4& view);
    void setProjectionMatrix(const glm::mat4& projection);
    void setCameraPosition(const glm::vec3& position);
    
    // Stats
    const FrameStats& getStats() const { return stats; }
    
    // Configuration
    const RendererConfig& getConfig() const { return config; }
    void updateConfig(const RendererConfig& newConfig);
    
    // Wait for GPU (for shutdown)
    void waitForIdle();
    
private:
    Renderer() = default;
    ~Renderer();
    
    void createSystems();
    void destroySystems();
    void setupRenderGraph();
    void updateStats();
    
    bool initialized = false;
    
    // Configuration
    RendererConfig config;
    FrameStats stats;
    
    // Core systems
    std::unique_ptr<VulkanContext> vulkanContext;
    std::unique_ptr<ResourceSystem> resourceSystem;
    std::unique_ptr<PipelineSystem> pipelineSystem;
    std::unique_ptr<RenderGraph> renderGraph;
    
    // 2D renderers
    std::unique_ptr<SpriteRenderer2D> spriteRenderer;
    std::unique_ptr<TilemapRenderer2D> tilemapRenderer;
    
    // 3D renderers
    std::unique_ptr<Renderer3D> renderer3D;
    std::unique_ptr<InstancedRenderer> instancedRenderer;
    std::unique_ptr<SkinnedMeshRenderer> skinnedMeshRenderer;
    
    // Effects
    std::unique_ptr<ParticleSystem> particleSystem;
    
    // Camera state
    glm::mat4 viewMatrix;
    glm::mat4 projectionMatrix;
    glm::vec3 cameraPosition;
    
    // Frame synchronization
    VkFence inFlightFences[3] = {};
    VkSemaphore imageAvailableSemaphores[3] = {};
    VkSemaphore renderFinishedSemaphores[3] = {};
    uint32_t currentFrame = 0;
    uint32_t currentImageIndex = 0;
};

} // namespace FarmEngine
