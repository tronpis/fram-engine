#include "Renderer.h"

namespace FarmEngine {

// Singleton implementation
Renderer& Renderer::getInstance() {
    static Renderer instance;
    return instance;
}

bool Renderer::initialize(void* windowHandle, void* windowInstance, 
                         const RendererConfig& cfg) {
    if (initialized) {
        return true;
    }
    
    config = cfg;
    
    // Initialize Vulkan context
    vulkanContext = std::make_unique<VulkanContext>();
    if (!vulkanContext->initialize(windowHandle, windowInstance)) {
        return false;
    }
    
    // Create resource system
    createSystems();
    
    // Setup render graph
    setupRenderGraph();
    
    initialized = true;
    return true;
}

void Renderer::shutdown() {
    if (!initialized) return;
    
    waitForIdle();
    
    destroySystems();
    
    vulkanContext.reset();
    
    initialized = false;
}

bool Renderer::beginFrame() {
    // Wait for previous frame
    vkWaitForFences(vulkanContext->getDevice(), 1, 
                   &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
    vkResetFences(vulkanContext->getDevice(), 1, &inFlightFences[currentFrame]);
    
    // Acquire next image
    VkResult result = vkAcquireNextImageKHR(
        vulkanContext->getDevice(),
        vulkanContext->getSwapchain(),
        UINT64_MAX,
        imageAvailableSemaphores[currentFrame],
        VK_NULL_HANDLE,
        &currentImageIndex
    );
    
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        // Handle resize
        return false;
    }
    
    stats.frameNumber++;
    return true;
}

void Renderer::endFrame() {
    // Submit command buffer (to be implemented with render graph)
    
    // Present
    present();
    
    currentFrame = (currentFrame + 1) % 3;
    
    updateStats();
}

void Renderer::present() {
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &renderFinishedSemaphores[currentFrame];
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &vulkanContext->getSwapchain();
    presentInfo.pImageIndices = &currentImageIndex;
    
    vkQueuePresentKHR(vulkanContext->getPresentQueue(), &presentInfo);
}

void Renderer::onWindowResize(uint32_t newWidth, uint32_t newHeight) {
    config.screenWidth = newWidth;
    config.screenHeight = newHeight;
    
    waitForIdle();
    
    // Recreate swapchain and framebuffers
    if (renderGraph) {
        renderGraph->resize(newWidth, newHeight);
    }
}

void Renderer::setViewMatrix(const glm::mat4& view) {
    viewMatrix = view;
}

void Renderer::setProjectionMatrix(const glm::mat4& projection) {
    projectionMatrix = projection;
}

void Renderer::setCameraPosition(const glm::vec3& position) {
    cameraPosition = position;
}

void Renderer::updateConfig(const RendererConfig& newConfig) {
    config = newConfig;
    // Apply changes to subsystems
}

void Renderer::waitForIdle() {
    if (vulkanContext && vulkanContext->getDevice()) {
        vkDeviceWaitIdle(vulkanContext->getDevice());
    }
}

Renderer::~Renderer() {
    shutdown();
}

void Renderer::createSystems() {
    auto device = vulkanContext->getDevice();
    auto physicalDevice = vulkanContext->getPhysicalDevice();
    
    // Resource system
    resourceSystem = std::make_unique<ResourceSystem>();
    resourceSystem->initialize(
        vulkanContext->getInstance(),
        physicalDevice,
        device,
        vulkanContext->getGraphicsQueue(),
        vulkanContext->getPresentQueue()  // Using present as transfer for now
    );
    
    // Pipeline system
    pipelineSystem = std::make_unique<PipelineSystem>();
    pipelineSystem->initialize(device, physicalDevice);
    
    // Render graph
    renderGraph = std::make_unique<RenderGraph>();
    renderGraph->initialize(device, physicalDevice, 
                           config.screenWidth, config.screenHeight);
    
    // 2D renderers
    spriteRenderer = std::make_unique<SpriteRenderer2D>(device, physicalDevice);
    tilemapRenderer = std::make_unique<TilemapRenderer2D>(device, physicalDevice);
    
    // 3D renderers
    renderer3D = std::make_unique<Renderer3D>();
    renderer3D->initialize(device, physicalDevice);
    
    instancedRenderer = std::make_unique<InstancedRenderer>(device, physicalDevice);
    instancedRenderer->initialize(config.maxVegetationInstances);
    
    skinnedMeshRenderer = std::make_unique<SkinnedMeshRenderer>(device, physicalDevice);
    skinnedMeshRenderer->initialize();
    
    // Particle system
    particleSystem = std::make_unique<ParticleSystem>(device, physicalDevice);
    particleSystem->initialize(config.maxParticles);
}

void Renderer::destroySystems() {
    // Order matters - destroy in reverse dependency order
    
    particleSystem.reset();
    skinnedMeshRenderer.reset();
    instancedRenderer.reset();
    renderer3D.reset();
    tilemapRenderer.reset();
    spriteRenderer.reset();
    
    renderGraph.reset();
    pipelineSystem.reset();
    resourceSystem.reset();
}

void Renderer::setupRenderGraph() {
    // Add passes to render graph
    auto* shadowPass = renderGraph->addShadowPass();
    auto* gbufferPass = renderGraph->addGBufferPass();
    auto* lightingPass = renderGraph->addLightingPass();
    auto* transparentPass = renderGraph->addTransparentPass();
    auto* postProcessPass = renderGraph->addPostProcessPass();
    auto* uiPass = renderGraph->addUIPass();
    
    // Configure passes based on settings
    lightingPass->setAmbientLight(glm::vec3(0.1f));
    
    postProcessPass->configure(
        config.enableBloom,
        true,  // tonemapping always on
        true,  // vignette
        false  // chromatic aberration
    );
    
    // Compile the graph
    renderGraph->compile();
}

void Renderer::updateStats() {
    static auto lastTime = std::chrono::high_resolution_clock::now();
    auto currentTime = std::chrono::high_resolution_clock::now();
    auto deltaTime = std::chrono::duration<float>(currentTime - lastTime).count();
    lastTime = currentTime;
    
stats.fps = (deltaTime > 0.0f) ? (1.0f / deltaTime) : 0.0f;
stats.frameTime = deltaTime * 1000.0f;

    
    // Gather stats from subsystems
    if (renderer3D) {
        stats.drawCalls = renderer3D->getDrawCalls();
        stats.triangleCount = renderer3D->getTriangleCount();
    }
    
    if (instancedRenderer) {
        stats.instanceCount = instancedRenderer->getTotalInstanceCount();
    }
    
    if (particleSystem) {
        stats.particleCount = particleSystem->getActiveParticleCount();
    }
    
    if (resourceSystem) {
        stats.gpuMemoryUsed = resourceSystem->getMemoryAllocator().getTotalAllocatedSize();
    }
}

} // namespace FarmEngine
