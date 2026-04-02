#pragma once

/**
 * @file ExampleUsage.h
 * @brief Ejemplos de uso del Render Graph para diferentes casos del FarmEngine
 */

#include "RenderGraph.h"
#include "RenderPipeline.h"

namespace FarmEngine::Render::Examples {

/**
 * @brief Ejemplo: Pipeline completo para rendering de chunks de terreno
 * 
 * Este es el caso de uso principal para FarmEngine - renderizar chunks
 * voxel con iluminación dinámica y oclusión ambiental.
 */
inline void ChunkRenderingExample(RenderGraph& graph, VkDevice device, VkExtent2D swapchainExtent) {
    RenderGraphBuilder builder;
    
    // 1. Recursos principales
    builder.addColorTarget("SceneColor", VK_FORMAT_R16G16B16A16_SFLOAT, swapchainExtent, ResourceLifetime::Frame);
    builder.addDepthTarget("SceneDepth", VK_FORMAT_D32_SFLOAT, swapchainExtent, ResourceLifetime::Frame);
    
    // 2. Geometry Pass - Renderiza todos los chunks visibles
    builder.addPass("ChunkGeometry", [](RenderPass& pass) {
        pass.colorAttachments.push_back("SceneColor");
        pass.depthAttachment = "SceneDepth";
        pass.clearColor = true;
        pass.clearDepth = true;
        pass.clearColorValue = {{0.4f, 0.6f, 0.9f, 1.0f}}; // Sky blue
        
        // Callback que se ejecutará durante el render pass
        pass.executeCallback = [](VkCommandBuffer cmd, const ResourceRegistry& registry) {
            // Aquí se integrarían con ChunkManager::getVisibleChunks()
            // for (auto* chunk : visibleChunks) {
            //     chunk->renderOpaque(cmd);
            // }
        };
    });
    
    // 3. Transparency Pass - Plantas, agua, partículas
    builder.addPass("Transparency", [](RenderPass& pass) {
        pass.colorAttachments.push_back("SceneColor");
        pass.depthAttachment = "SceneDepth";
        pass.colorLoadOp = VK_ATTACHMENT_LOAD_OP_LOAD; // Preserve previous pass
        pass.depthLoadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        
        pass.executeCallback = [](VkCommandBuffer cmd, const ResourceRegistry& registry) {
            // Render transparent crops, water, animals
        };
    });
    
    // 4. Post-Processing - Tonemapping a swapchain
    builder.addPass("Tonemapping", [](RenderPass& pass) {
        pass.inputAttachments = {"SceneColor"};
        pass.colorAttachments.push_back("Swapchain"); // External
        pass.clearColor = false;
        
        pass.executeCallback = [](VkCommandBuffer cmd, const ResourceRegistry& registry) {
            // ACES tonemapping + gamma correction
        };
    });
    
    graph.compile(std::move(builder));
    graph.build(device, swapchainExtent);
}

/**
 * @brief Ejemplo: Shadow mapping para iluminación dinámica
 * 
 * Muestra cómo añadir un pass de shadow map antes del geometry pass
 */
inline void ShadowMappingExample(RenderGraph& graph, VkDevice device, VkExtent2D extent) {
    RenderGraphBuilder builder;
    
    // Shadow map resource (alta resolución para calidad)
    builder.addDepthTarget("ShadowMap", VK_FORMAT_D32_SFLOAT, {2048, 2048}, ResourceLifetime::Frame);
    
    // Shadow pass - renderiza desde perspectiva de la luz
    builder.addPass("ShadowPass", [](RenderPass& pass) {
        pass.depthAttachment = "ShadowMap";
        pass.clearDepth = true;
        pass.clearDepthValue = {1.0f, 0};
        
        pass.executeCallback = [](VkCommandBuffer cmd, const ResourceRegistry& registry) {
            // Bind shadow pipeline
            // Set light view/projection matrices
            // Render depth-only pass de chunks
        };
    });
    
    // Main geometry pass que usa el shadow map
    builder.addPass("MainGeometry", [](RenderPass& pass) {
        pass.colorAttachments.push_back("SceneColor");
        pass.depthAttachment = "SceneDepth";
        pass.inputAttachments.push_back("ShadowMap"); // Para sampling en shader
        
        pass.executeCallback = [](VkCommandBuffer cmd, const ResourceRegistry& registry) {
            // Fragment shader samplea ShadowMap para calcular oclusión
        };
    });
    
    // Dependencia explícita: ShadowPass -> MainGeometry
    builder.addDependency(
        0, 1, // From pass 0 to pass 1
        VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
        VK_ACCESS_SHADER_READ_BIT
    );
    
    graph.compile(std::move(builder));
    graph.build(device, extent);
}

/**
 * @brief Ejemplo: SSAO (Screen Space Ambient Occlusion)
 * 
 * Técnica post-process para oclusión ambiental en tiempo real
 */
inline void SSAOExample(RenderGraph& graph, VkDevice device, VkExtent2D extent) {
    RenderGraphBuilder builder;
    
    // GBuffer básico
    builder.addColorTarget("GBuffer_Albedo", VK_FORMAT_R8G8B8A8_UNORM, extent, ResourceLifetime::Frame);
    builder.addColorTarget("GBuffer_Normals", VK_FORMAT_A2B10G10R10_UNORM_PACK32, extent, ResourceLifetime::Frame);
    builder.addDepthTarget("SceneDepth", VK_FORMAT_D32_SFLOAT, extent, ResourceLifetime::Frame);
    
    // SSAO buffer (half-res para performance)
    builder.addColorTarget("SSAO_Buffer", VK_FORMAT_R8_UNORM, {extent.width / 2, extent.height / 2}, ResourceLifetime::Frame);
    
    // 1. Geometry
    builder.addPass("Geometry", [](RenderPass& pass) {
        pass.colorAttachments = {"GBuffer_Albedo", "GBuffer_Normals"};
        pass.depthAttachment = "SceneDepth";
        // ... configurar callbacks
    });
    
    // 2. SSAO Pass
    builder.addPass("SSAO", [](RenderPass& pass) {
        pass.inputAttachments = {"GBuffer_Normals", "SceneDepth"};
        pass.colorAttachments.push_back("SSAO_Buffer");
        pass.clearColor = true;
        
        pass.executeCallback = [](VkCommandBuffer cmd, const ResourceRegistry& registry) {
            // Full-screen quad con SSAO calculation
            // Samplea normals + depth para calcular oclusión
        };
    });
    
    // 3. Lighting con SSAO aplicado
    builder.addPass("Lighting", [](RenderPass& pass) {
        pass.inputAttachments = {"GBuffer_Albedo", "GBuffer_Normals", "SSAO_Buffer"};
        pass.colorAttachments.push_back("FinalColor");
        
        pass.executeCallback = [](VkCommandBuffer cmd, const ResourceRegistry& registry) {
            // Lighting calculation con SSAO factor
        };
    });
    
    graph.compile(std::move(builder));
    graph.build(device, extent);
}

/**
 * @brief Ejemplo: Minimizado para testing
 * 
 * Pipeline más simple posible para validar integración
 */
inline void MinimalExample(RenderGraph& graph, VkDevice device, VkExtent2D extent) {
    RenderGraphBuilder builder;
    
    builder.addColorTarget("Backbuffer", VK_FORMAT_B8G8R8A8_SRGB, extent, ResourceLifetime::External);
    builder.addDepthTarget("Depth", VK_FORMAT_D32_SFLOAT, extent, ResourceLifetime::Frame);
    
    builder.addPass("ClearAndDraw", [](RenderPass& pass) {
        pass.colorAttachments.push_back("Backbuffer");
        pass.depthAttachment = "Depth";
        pass.clearColor = true;
        pass.clearDepth = true;
        
        pass.executeCallback = [](VkCommandBuffer cmd, const ResourceRegistry& registry) {
            // Draw single triangle or quad
        };
    });
    
    graph.compile(std::move(builder));
    graph.build(device, extent);
}

} // namespace FarmEngine::Render::Examples
