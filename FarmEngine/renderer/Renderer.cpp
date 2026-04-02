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
    
    // Create synchronization objects
    auto device = vulkanContext->getDevice();
    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        // Create fence
        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;  // Start signaled to avoid blocking on first frame
        if (vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {
            return false;
        }
        
        // Create image available semaphore
        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS) {
            return false;
        }
        
        // Create render finished semaphore
        if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS) {
            return false;
        }
    }
    
    // Create resource system
    if (!createSystems()) {
        return false;
    }
    
    // Setup render graph
    if (!setupRenderGraph()) {
        destroySystems();
        return false;
    }
    
    initialized = true;
    return true;
}

void Renderer::shutdown() {
    if (!initialized) return;
    
    waitForIdle();
    
    // Destroy synchronization objects
    auto device = vulkanContext->getDevice();
    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        vkDestroyFence(device, inFlightFences[i], nullptr);
        vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
        vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
    }
    
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
    
if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
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
    
    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    
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
    
    VkResult result = vkQueuePresentKHR(vulkanContext->getPresentQueue(), &presentInfo);
    
    // Handle swapchain recreation scenarios
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        // Swapchain needs recreation - will be handled by resize or next frame
        return;
    } else if (result != VK_SUCCESS) {
        // Other errors (e.g., device lost) should be handled appropriately
        // For now, we just log it - in production you'd want more robust handling
    }
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

bool Renderer::createSystems() {
    auto device = vulkanContext->getDevice();
    auto physicalDevice = vulkanContext->getPhysicalDevice();
    
    // Resource system
    resourceSystem = std::make_unique<ResourceSystem>();
    resourceSystem->initialize(
        vulkanContext->getInstance(),
        physicalDevice,
        device,
        vulkanContext->getGraphicsQueue(),
        vulkanContext->getGraphicsQueue()  // Using graphics queue for transfer (guaranteed to support it)
    );
    
    // Pipeline system
    pipelineSystem = std::make_unique<PipelineSystem>();
    if (!pipelineSystem->initialize(device, physicalDevice)) {
        return false;
    }
    
    // Render graph
    renderGraph = std::make_unique<RenderGraph>();
    if (!renderGraph->initialize(device, physicalDevice, 
                           config.screenWidth, config.screenHeight)) {
        return false;
    }
    
    // 2D renderers - initialize with default parameters
    spriteRenderer = std::make_unique<SpriteRenderer2D>(device, physicalDevice);
    if (!spriteRenderer->initialize()) {
        return false;
    }
    
    // TilemapRenderer requires specific dimensions - initialize on-demand
    // We'll create it here but defer initialization until first use
    tilemapRenderer = std::make_unique<TilemapRenderer2D>(device, physicalDevice);
    // Note: Call tilemapRenderer->initialize(width, height, tileSize) before first use
    
    // 3D renderers
    renderer3D = std::make_unique<Renderer3D>();
    if (!renderer3D->initialize(device, physicalDevice)) {
        return false;
    }
    
    instancedRenderer = std::make_unique<InstancedRenderer>(device, physicalDevice);
    if (!instancedRenderer->initialize(config.maxVegetationInstances)) {
        return false;
    }
    
    skinnedMeshRenderer = std::make_unique<SkinnedMeshRenderer>(device, physicalDevice);
    if (!skinnedMeshRenderer->initialize()) {
        return false;
    }
    
    // Particle system
    particleSystem = std::make_unique<ParticleSystem>(device, physicalDevice);
    if (!particleSystem->initialize(config.maxParticles)) {
        return false;
    }
    
    return true;
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

bool Renderer::setupRenderGraph() {
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
    
    return true;
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
