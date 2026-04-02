# Render Graph Module - FarmEngine

## Overview

Sistema de renderizado basado en **Render Graph** para Vulkan 1.2+, siguiendo las mejores prácticas de motores modernos (Unreal, Frostbite, id Tech).

## Ventajas Clave

### 1. Sincronización Automática
- Barreras de memoria generadas automáticamente
- Semaphores entre passes
- Transiciones de layout implícitas

### 2. Optimización TBDR (Tile-Based Deferred Rendering)
- Reordenamiento de passes para maximizar coherencia de tiles
- Minimización de bandwidth de memoria
- Ideal para GPUs mobile (PowerVR, Adreno, Mali)

### 3. Alias de Memoria
- Reutilización automática de recursos temporales
- Recursos con lifetime `Frame` se reciclan entre frames
- Reducción significativa de VRAM usage

## Arquitectura

```
┌─────────────────────────────────────────────────────────────┐
│                    RenderGraph                               │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐       │
│  │ GeometryPass │→ │ LightingPass │→ │ PostProcess  │       │
│  └──────────────┘  └──────────────┘  └──────────────┘       │
│         ↓                  ↓                  ↓              │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐       │
│  │ GBuffer      │  │ HDR Buffer   │  │ Swapchain    │       │
│  └──────────────┘  └──────────────┘  └──────────────┘       │
└─────────────────────────────────────────────────────────────┘
```

## Uso Básico

```cpp
#include "rendering/graph/RenderGraph.h"
#include "rendering/graph/RenderPipeline.h"

// Crear grafo
RenderGraph graph;

// Configurar forward renderer
ForwardRenderer renderer;
renderer.buildGraph(graph, swapchainExtent);

// Crear recursos Vulkan
graph.createRenderPasses(device);
graph.createFramebuffers(device, swapchainExtent);

// Ejecutar cada frame
void renderFrame(VkCommandBuffer cmd) {
    ResourceRegistry registry = graph.getRegistry();
    graph.execute(cmd, registry, frameIndex);
}
```

## Deferred Rendering Avanzado

```cpp
DeferredRenderer deferred;
deferred.buildGraph(graph, swapchainExtent);

// GBuffer structure:
// - Albedo:     VK_FORMAT_R8G8B8A8_UNORM
// - Normals:    VK_FORMAT_A2B10G10R10_UNORM_PACK32
// - Depth:      VK_FORMAT_R32_SFLOAT
// - Material:   VK_FORMAT_R8G8B8A8_UNORM
// - HDR Light:  VK_FORMAT_R16G16B16A16_SFLOAT
```

## Tipos de Recursos

| Lifetime | Descripción | Ejemplo |
|----------|-------------|---------|
| `Frame` | Se crea/destruye cada frame | GBuffer temporal, HDR buffer |
| `Persistent` | Vive toda la vida del graph | Texturas cargadas, buffers estáticos |
| `External` | Gestionado externamente | Swapchain images, recursos del engine |

## Integración con ChunkSystem

El Render Graph se integra naturalmente con el sistema de chunks:

```cpp
builder.addPass("ChunkGeometry", [](RenderPass& pass) {
    pass.colorAttachments.push_back("GBuffer_Albedo");
    pass.depthAttachment = "Depth";
    
    pass.executeCallback = [chunkManager](VkCommandBuffer cmd, const ResourceRegistry& reg) {
        auto visibleChunks = chunkManager.getVisibleChunks();
        for (auto* chunk : visibleChunks) {
            chunk->render(cmd);
        }
    };
});
```

## Próximas Mejoras

- [ ] Inferencia automática de dependencias
- [ ] Async compute queue support
- [ ] Frame graph statistics & profiling
- [ ] Hot-reload de passes desde ImGui
- [ ] Memory alias analysis optimizer

## Referencias

- [GPUOpen Render Graph](https://gpuopen.com/)
- [Frostbite Render Graph Architecture](https://www.ea.com/seeds/pubs)
- [Vulkan Best Practices](https://github.com/KhronosGroup/Vulkan-Guide)
