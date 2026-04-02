#include "RenderPipeline.h"

namespace FarmEngine::Render {

// ============================================================================
// ForwardRenderer Implementation
// ============================================================================

ForwardRenderer::ForwardRenderer() : extent{0, 0} {}

void ForwardRenderer::buildGraph(RenderGraph& graph, VkExtent2D swapchainExtent) {
    extent = swapchainExtent;
    
    RenderGraphBuilder builder;
    
    // 1. Crear recursos
    builder.addColorTarget("FinalColor", VK_FORMAT_B8G8R8A8_SRGB, extent, ResourceLifetime::External);
    builder.addDepthTarget("Depth", VK_FORMAT_D32_SFLOAT, extent, ResourceLifetime::Frame);
    
    // 2. Configurar Geometry Pass
    builder.addPass("Geometry", [](RenderPass& pass) {
        pass.colorAttachments.push_back("FinalColor");
        pass.depthAttachment = "Depth";
        pass.clearColor = true;
        pass.clearDepth = true;
        pass.clearColorValue = {{0.0f, 0.0f, 0.0f, 1.0f}};
        pass.clearDepthValue = {1.0f, 0};
        
        pass.executeCallback = [](VkCommandBuffer cmd, const ResourceRegistry& registry) {
            // Aquí iría el draw call de geometría
            // vkCmdDrawIndexed(...);
        };
    });
    
    // Compilar grafo
    graph.compile(std::move(builder));
}

void ForwardRenderer::setupPasses() {
    // Configuración específica de pipelines, descriptor sets, etc.
}

// ============================================================================
// DeferredRenderer Implementation
// ============================================================================

DeferredRenderer::DeferredRenderer() : extent{0, 0} {}

void DeferredRenderer::buildGraph(RenderGraph& graph, VkExtent2D swapchainExtent) {
    extent = swapchainExtent;
    
    RenderGraphBuilder builder;
    
    // Setup GBuffer (múltiples attachments)
    setupGBuffer(builder);
    
    // Lighting Pass (lee del GBuffer, escribe en HDR buffer)
    setupLightingPass(builder);
    
    // Post-Processing (tonemapping, bloom, etc.)
    setupPostProcess(builder);
    
    graph.compile(std::move(builder));
}

void DeferredRenderer::setupGBuffer(RenderGraphBuilder& builder) {
    // GBuffer attachments típicos:
    // 1. Albedo (color + alpha)
    builder.addColorTarget("GBuffer_Albedo", VK_FORMAT_R8G8B8A8_UNORM, extent, ResourceLifetime::Frame);
    
    // 2. Normales (empacadas en RGB)
    builder.addColorTarget("GBuffer_Normals", VK_FORMAT_A2B10G10R10_UNORM_PACK32, extent, ResourceLifetime::Frame);
    
    // 3. Position/Depth
    builder.addColorTarget("GBuffer_Depth", VK_FORMAT_R32_SFLOAT, extent, ResourceLifetime::Frame);
    
    // 4. Material properties (roughness, metallic, etc.)
    builder.addColorTarget("GBuffer_Material", VK_FORMAT_R8G8B8A8_UNORM, extent, ResourceLifetime::Frame);
    
    // Depth buffer compartido
    builder.addDepthTarget("Depth", VK_FORMAT_D32_SFLOAT, extent, ResourceLifetime::Frame);
    
    // Geometry Pass que escribe en todos los GBuffer
    builder.addPass("GBuffer_Geometry", [](RenderPass& pass) {
        pass.colorAttachments = {"GBuffer_Albedo", "GBuffer_Normals", "GBuffer_Depth", "GBuffer_Material"};
        pass.depthAttachment = "Depth";
        pass.clearColor = true;
        pass.clearDepth = true;
        
        pass.executeCallback = [](VkCommandBuffer cmd, const ResourceRegistry& registry) {
            // Draw geometry to all GBuffer attachments simultaneously
            // Vulkan permite hasta 8 color attachments en un solo pass
        };
    });
}

void DeferredRenderer::setupLightingPass(RenderGraphBuilder& builder) {
    // HDR buffer para acumular luz
    builder.addColorTarget("HDR_Lighting", VK_FORMAT_R16G16B16A16_SFLOAT, extent, ResourceLifetime::Frame);
    
    builder.addPass("Lighting", [](RenderPass& pass) {
        // Lee del GBuffer como input attachments (texture sampling desde fragment shader)
        pass.inputAttachments = {"GBuffer_Albedo", "GBuffer_Normals", "GBuffer_Depth", "GBuffer_Material"};
        pass.colorAttachments.push_back("HDR_Lighting");
        pass.clearColor = true;
        pass.clearColorValue = {{0.0f, 0.0f, 0.0f, 1.0f}};
        
        pass.executeCallback = [](VkCommandBuffer cmd, const ResourceRegistry& registry) {
            // Full-screen quad con lighting calculations
            // Fragment shader muestrea los input attachments para calcular iluminación
        };
    });
}

void DeferredRenderer::setupPostProcess(RenderGraphBuilder& builder) {
    // Buffer intermedio para post-processing
    builder.addColorTarget("PostProcess_Temp", VK_FORMAT_R16G16B16A16_SFLOAT, extent, ResourceLifetime::Frame);
    
    // Salida final (swapchain)
    // builder.addExternalImage se hace fuera, aquí solo referenciamos
    
    builder.addPass("Tonemapping", [](RenderPass& pass) {
        pass.inputAttachments = {"HDR_Lighting"};
        pass.colorAttachments.push_back("FinalColor"); // External swapchain
        pass.clearColor = false;
        
        pass.executeCallback = [](VkCommandBuffer cmd, const ResourceRegistry& registry) {
            // ACES tonemapping, bloom, color grading, etc.
        };
    });
}

// ============================================================================
// ResourceFactory Implementation
// ============================================================================

VkImageCreateInfo ResourceFactory::createImageInfo(
    VkFormat format,
    VkExtent2D extent,
    VkImageUsageFlags usage,
    uint32_t arrayLayers,
    uint32_t mipLevels
) {
    VkImageCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    info.imageType = VK_IMAGE_TYPE_2D;
    info.format = format;
    info.extent.width = extent.width;
    info.extent.height = extent.height;
    info.extent.depth = 1;
    info.mipLevels = mipLevels;
    info.arrayLayers = arrayLayers;
    info.samples = VK_SAMPLE_COUNT_1_BIT;
    info.tiling = VK_IMAGE_TILING_OPTIMAL;
    info.usage = usage;
    info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
    return info;
}

VkImageViewCreateInfo ResourceFactory::createImageViewInfo(
    VkImage image,
    VkFormat format,
    VkImageAspectFlags aspectMask
) {
    VkImageViewCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    info.image = image;
    info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    info.format = format;
    info.subresourceRange.aspectMask = aspectMask;
    info.subresourceRange.baseMipLevel = 0;
    info.subresourceRange.levelCount = 1;
    info.subresourceRange.baseArrayLayer = 0;
    info.subresourceRange.layerCount = 1;
    
    return info;
}

} // namespace FarmEngine::Render
