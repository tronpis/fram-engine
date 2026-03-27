# FarmEngine - Arquitectura de Motor Vulkan Moderno

## 🎯 Visión General

FarmEngine es un motor de juego especializado en simulación agrícola con arquitectura modular tipo Unreal y renderizado Vulkan moderno.

**Estado Actual:** El repositorio ya contiene una estructura completa organizada por sistemas.

---

## 📁 Estructura del Proyecto

```
FarmEngine/
├── core/                    # Núcleo del engine
│   ├── Engine.h/.cpp        # Clase principal del motor
│   ├── ecs/                 # Entity Component System
│   ├── jobsystem/           # Sistema de trabajos multihilo
│   ├── logger/              # Sistema de logging
│   └── memory/              # Gestor de memoria personalizado
│
├── renderer/                # Sistema de renderizado Vulkan
│   ├── vulkan/              # Contexto y dispositivos Vulkan
│   ├── passes/              # Render Passes (Shadow, GBuffer, etc.)
│   ├── 2d/                  # Renderer 2D (sprites, tilemaps)
│   └── 3d/                  # Renderer 3D (meshes, terrain)
│
├── world/                   # Gestión del mundo
│   ├── chunks/              # Sistema de chunks para streaming
│   ├── terrain/             # Generación y render de terreno
│   └── tiles/               # Tilemap para granjas 2D
│
├── simulation/              # Lógica de simulación agrícola
│   ├── crops/               # Sistema de cultivos
│   ├── animals/             # IA de animales
│   ├── soil/                # Simulación de suelo
│   └── weather/             # Clima y estaciones
│
├── physics/                 # Motor de física
├── audio/                   # Sistema de audio
├── platform/                # Capa de abstracción de plataforma
│   ├── window/              # Gestión de ventanas (GLFW/SDL)
│   └── input/               # Sistema de input
│
├── tools/                   # Herramientas de desarrollo
│   ├── editor/              # Editor integrado (ImGui)
│   └── inspector/           # Inspector de entidades
│
├── plugins/                 # Sistema de plugins/mods
└── assets/                  # Recursos del juego
    ├── shaders/             # Shaders GLSL
    ├── textures/            # Texturas
    └── meshes/              # Modelos 3D
```

---

## 🏗️ Arquitectura Modular

El engine usa un sistema de módulos interconectados:

### 1. **Core Module** (Núcleo)
- **Engine**: Loop principal, gestión de tiempo
- **ECS**: Entity Component System para entidades
- **JobSystem**: Paralelización de tareas
- **Memory**: Allocators personalizados (stack, pool, frame)
- **Logger**: Sistema de logs jerárquico

### 2. **Renderer Module** (Vulkan)
```
Renderer
├── VulkanContext
│   ├── Instance
│   ├── PhysicalDevice
│   ├── LogicalDevice
│   ├── Swapchain
│   └── Queues (Graphics, Compute, Transfer)
│
├── ResourceSystem
│   ├── BufferPool (Vertex, Index, Uniform)
│   ├── TexturePool
│   ├── DescriptorAllocator
│   └── MemoryAllocator (VMA-like)
│
├── PipelineSystem
│   ├── ShaderCompiler (SPIR-V)
│   ├── PipelineCache
│   └── PipelineLayouts
│
├── RenderGraph
│   ├── ShadowPass
│   ├── GBufferPass
│   ├── LightingPass
│   ├── TransparentPass
│   ├── PostProcessPass
│   └── UIPass
│
├── Renderer2D
│   ├── SpriteBatcher
│   ├── TilemapRenderer
│   └── ParticleSystem
│
└── Renderer3D
    ├── MeshRenderer
    ├── TerrainRenderer
    ├── InstancedRenderer (vegetación)
    └── SkinnedMesh (animales)
```

### 3. **World Module**
- **ChunkSystem**: División del mundo en chunks (32x32 tiles)
- **Streaming**: Carga/descarga dinámica según posición del jugador
- **LOD**: Niveles de detalle para terreno y vegetación
- **Culling**: Frustum + Distance culling

### 4. **Simulation Module**
- **CropSystem**: Crecimiento por días (no por frame)
- **AnimalSystem**: IA simple (wander, eat, sleep, produce)
- **SoilSystem**: Moisture, fertility, tilled state
- **WeatherSystem**: Rain, sun, seasons, temperature

---

## 🔧 Tecnologías Base

| Sistema | Tecnología | Propósito |
|---------|-----------|-----------|
| **API Gráfica** | Vulkan 1.2+ | Renderizado de alto rendimiento |
| **Ventanas** | GLFW o SDL2 | Creación de ventana y contexto |
| **Math** | GLM | Matemáticas (vec3, mat4, quat) |
| **Assets** | stb_image | Carga de texturas (PNG, JPG) |
| **Modelos** | tinygltf | Carga de modelos 3D (glTF) |
| **UI** | Dear ImGui | Editor y herramientas debug |
| **Física** | Box2D / Bullet | Colisiones 2D/3D |
| **Audio** | miniaudio / FMOD | Sonido y música |

---

## 🚀 Conceptos Clave de Diseño

### 1. **Render Graph**
Los motores modernos NO usan draw calls directos. Usan un grafo de render:

```cpp
struct RenderPassNode {
    std::vector<Texture> inputs;
    std::vector<Texture> outputs;
    std::function<void(VkCommandBuffer)> execute;
};

// Ejemplo de grafo
RenderGraph graph;
graph.addPass("Shadow", shadowInputs, shadowOutputs, renderShadows);
graph.addPass("GBuffer", gbufIn, gbufOut, renderGeometry);
graph.addPass("Lighting", lightIn, lightOut, computeLighting);
graph.compile(); // Optimiza dependencias
graph.execute(); // Ejecuta en orden óptimo
```

**Ventajas:**
- Sincronización automática (semaphores, barriers)
- Reordenamiento de passes para optimizar TBDR
- Alias de memoria (reutilizar recursos temporales)

### 2. **GPU Instancing**
Para renderizar 100k plantas en 1 draw call:

```cpp
struct PlantInstance {
    vec3 position;
    float rotation;
    float scale;
    float growthStage;
};

// CPU: Preparar buffer de instancias
std::vector<PlantInstance> instances(100000);
fillInstanceData(instances);

// GPU: Single draw call
vkCmdBindVertexBuffers(cmd, 0, 1, &instanceBuffer, &offset);
vkCmdDrawIndexed(cmd, indexCount, instanceCount, 0, 0, 0);
```

### 3. **Simulación por Días**
Los cultivos NO se actualizan cada frame:

```cpp
// Frame update (60 FPS)
void update() {
    processInput();
    updatePlayerMovement();
    render();
}

// Day update (cada 60 segundos reales = 1 día juego)
void onDayChanged() {
    for (auto& crop : allCrops) {
        if (crop.water > threshold) {
            crop.growthStage++;
        }
    }
    updateAnimals();
    spawnRandomEvent();
}
```

**Beneficio:** Reduce CPU workload 1000x

### 4. **Chunk-Based Streaming**
Mundos infinitos tipo Minecraft:

```
Chunks cargados: 9 (3x3 alrededor del jugador)
Chunks totales: 5000+
Tamaño chunk: 32x32 tiles

[ ][ ][ ]
[ ][P][ ]  <- Jugador en centro
[ ][ ][ ]
```

```cpp
void updateStreaming(vec3 playerPos) {
    ChunkID currentChunk = getChunkAt(playerPos);
    
    // Cargar chunks cercanos
    for (int dx = -1; dx <= 1; dx++) {
        for (int dy = -1; dy <= 1; dy++) {
            loadChunk(currentChunk + ivec2(dx, dy));
        }
    }
    
    // Descargar lejanos
    unloadDistantChunks(playerPos, maxDistance);
}
```

### 5. **ECS (Entity Component System)**
Todo es una entidad con componentes:

```cpp
// Componentes (solo datos)
struct Transform { vec3 position; quat rotation; vec3 scale; };
struct Crop { int stage; float water; int daysToGrow; };
struct Animal { float hunger; float happiness; AIState state; };

// Sistemas (solo lógica)
class CropSystem : public System {
    void update(float dt) {
        auto view = registry.view<Crop, Transform>();
        for (auto entity : view) {
            auto& crop = view.get<Crop>(entity);
            crop.water -= dt * evaporationRate;
        }
    }
};
```

---

## 📊 Objetivos de Rendimiento

| Elemento | Cantidad | Técnica | Draw Calls |
|----------|----------|---------|------------|
| Cultivos | 100,000 | GPU Instancing + Culling | ~10 |
| Árboles | 10,000 | LOD + Billboard | ~50 |
| Animales | 500 | ECS + Job System | ~20 |
| Tiles 2D | 500,000 | Batch Rendering + Atlas | ~5 |
| Partículas | 10,000 | Compute Shader | 1 |

---

## 🛠️ Próximos Pasos (Roadmap)

### Fase 1: Vulkan Base ⏳
- [ ] Crear VulkanContext (Instance, Device, Swapchain)
- [ ] Implementar Frame System (2-3 frames in flight)
- [ ] Crear Resource Manager (buffers, textures)
- [ ] Compilar primer triángulo

### Fase 2: Render Graph
- [ ] Diseñar sistema de nodos
- [ ] Implementar Shadow Pass
- [ ] Implementar GBuffer Pass
- [ ] Implementar Lighting Pass

### Fase 3: Renderer 2D
- [ ] Sprite Batcher (atlas + batching)
- [ ] Tilemap Renderer (chunks visibles)
- [ ] Particle System (GPU-based)

### Fase 4: Renderer 3D
- [ ] Mesh Renderer (glTF loader)
- [ ] Terrain Renderer (heightmap + LOD)
- [ ] Instanced Renderer (vegetación masiva)

### Fase 5: Simulación
- [ ] Crop Growth System (data-driven)
- [ ] Animal AI (state machine)
- [ ] Soil Simulation (moisture, nutrients)
- [ ] Weather & Seasons

### Fase 6: Herramientas
- [ ] Editor con Dear ImGui
- [ ] Tile Painter
- [ ] Entity Inspector
- [ ] Crop Configurator (JSON)

---

## 💡 Recomendaciones de Diseño
1. **No empieces con Vulkan directo:** Usa una librería como `vk-bootstrap` para inicialización.
2. **Valida todo:** Usa Validation Layers desde el día 1.
3. **No reinventes la rueda:** Usa VMA (Vulkan Memory Allocator), no escribas el tuyo.
4. **Debug con RenderDoc:** Es tu mejor amigo para ver qué pasa en la GPU.
5. **Empieza pequeño:** Triángulo → Cuadrado → Sprite → Tilemap → 3D.
6. **Data-driven:** Los cultivos, animales, items deben estar en JSON, no hardcodeados.

---

## 📚 Recursos Recomendados

- [Vulkan Tutorial](https://vulkan-tutorial.com/) - La biblia de Vulkan
- [Render Graph Architecture](https://blog.logangrimm.com/render-graphs/)
- [ECS Design Patterns](https://dataorienteddesign.com/ecslite/)
- [GPU Instancing NVIDIA](https://developer.nvidia.com/gpugems/gpugems/part-vi-gpu-computation/chapter-33-massive-autonomous-agent-crowds-nvidia)
- [Vulkan Memory Allocator](https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator)
- [vk-bootstrap](https://github.com/charles-lunarg/vk-bootstrap)

---

## ⚠️ Consideraciones de Desarrollo

Crear un engine Vulkan completo puede tomar:
- **Mínimo:** 150,000 líneas de código
- **Tiempo:** 2-5 años (desarrollo part-time)
- **Complejidad:** AAA-level engineering

**¿Vale la pena?** 
- Si quieres hacer un juego → Usa Godot/Unity
- Si quieres aprender ingeniería de motores → Sí, absolutamente

---

*FarmEngine - Un motor de juego moderno para aprender ingeniería de software de alto nivel.* 🌾💻🚀
