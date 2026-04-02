#pragma once

#include "RenderGraph.h"
#include <vulkan/vulkan.h>

namespace FarmEngine::Render {

/**
 * @brief Ejemplo de configuración de un pipeline de renderizado moderno
 * 
 * Este ejemplo muestra cómo el Render Graph permite:
 * 1. Sincronización automática entre passes
 * 2. Reordenamiento para optimización TBDR (Tile-Based Deferred Rendering)
 * 3. Alias de memoria para recursos temporales
 */
class ForwardRenderer {
public:
    ForwardRenderer();
    
    /**
     * @brief Construye el grafo de render para forward rendering
     * 
     * Pipeline:
     * 1. GBuffer Pass (opcional, para deferred)
     * 2. Geometry Pass
     * 3. Lighting Pass
     * 4. Post-Processing
     * 5. Present
     */
    void buildGraph(RenderGraph& graph, VkDevice device, VkExtent2D swapchainExtent);
    
    /**
     * @brief Configura los passes específicos del renderer
     */
    void setupPasses();
    
private:
    VkExtent2D extent;
};

/**
 * @brief Ejemplo avanzado: Deferred Renderer con múltiples GBuffer
 * 
 * Demuestra alias de memoria y reutilización de recursos temporales
 */
class DeferredRenderer {
public:
    DeferredRenderer();
    
    void buildGraph(RenderGraph& graph, VkDevice device, VkExtent2D swapchainExtent);
    
    /**
     * Estructura típica de GBuffer:
     * - Albedo (RGBA8)
     * - Normals (RGB10_A2 o RGBA16F)
     * - Position/Depth (R32F o D24S8)
     * - Material (RGBA8)
     */
    void setupGBuffer(RenderGraphBuilder& builder);
    void setupLightingPass(RenderGraphBuilder& builder);
    void setupPostProcess(RenderGraphBuilder& builder);
    
private:
    VkExtent2D extent;
};

/**
 * @brief Helper para crear recursos Vulkan compatibles con el Render Graph
 */
class ResourceFactory {
public:
    static VkImageCreateInfo createImageInfo(
        VkFormat format,
        VkExtent2D extent,
        VkImageUsageFlags usage,
        uint32_t arrayLayers = 1,
        uint32_t mipLevels = 1
    );
    
    static VkImageViewCreateInfo createImageViewInfo(
        VkImage image,
        VkFormat format,
        VkImageAspectFlags aspectMask
    );
};

} // namespace FarmEngine::Render
