# FarmEngine - Modern Vulkan Game Engine for Farming Games

## 🎯 Overview

FarmEngine is a high-performance, modular game engine specialized for farming simulation games. 
Built with modern Vulkan rendering architecture, ECS-based design, and optimized for massive 
agricultural simulations (100k+ crops, thousands of animals).

**Key Features:**
- ✅ Modern Vulkan renderer with Render Graph architecture
- ✅ Hybrid 2D/3D rendering pipeline
- ✅ ECS (Entity Component System) architecture
- ✅ Massive crop simulation (GPU instancing, chunk-based)
- ✅ Modular plugin system (Unreal-style)
- ✅ Integrated editor with Dear ImGui
- ✅ World streaming & chunk system
- ✅ Job system for multithreading

---

## 🏗️ Architecture

```
FarmEngine/
├── core/           # Engine core systems
│   ├── memory/     # Custom allocators, memory pools
│   ├── logger/     # Logging system
│   ├── jobsystem/  # Multithreading & job scheduling
│   └── ecs/        # Entity Component System
│
├── platform/       # Platform abstraction
│   ├── window/     # Window management (GLFW/SDL)
│   └── input/      # Input handling
│
├── renderer/       # Rendering system
│   ├── vulkan/     # Vulkan backend
│   │   ├── context/    # Device, instance, swapchain
│   │   ├── resource/   # Buffer & texture management
│   │   ├── pipeline/   # Pipeline creation & caching
│   │   └── graph/      # Render Graph system
│   ├── passes/     # Render passes (Shadow, GBuffer, etc.)
│   ├── 2d/         # 2D sprite & tile renderer
│   └── 3d/         # 3D mesh & terrain renderer
│
├── world/          # World management
│   ├── terrain/    # Terrain generation & rendering
│   ├── chunks/     # Chunk system for streaming
│   └── tiles/      # Tilemap system
│
├── simulation/     # Game simulation
│   ├── crops/      # Crop growth system
│   ├── animals/    # Animal AI & behavior
│   ├── weather/    # Weather & seasons
│   └── soil/       # Soil simulation
│
├── physics/        # Physics engine integration
├── audio/          # Audio system
├── tools/          # Development tools
│   ├── editor/     # Integrated game editor
│   └── inspector/  # Entity inspector
│
├── assets/         # Game assets
│   ├── shaders/    # Vulkan shaders (GLSL/HLSL)
│   ├── textures/   # Texture atlases
│   └── meshes/     # 3D models
│
└── plugins/        # Plugin system
```

---

## 🚀 Core Systems

### 1. Vulkan Renderer

Modern render graph architecture:

```cpp
struct RenderGraph {
    std::vector<RenderPassNode> passes;
    
    // Passes: Shadow → GBuffer → Lighting → Transparent → UI
    void addPass(const std::string& name, RenderPassFunction func);
    void execute(VkCommandBuffer cmd);
};
```

**Frame System (3 frames in flight):**
```cpp
struct FrameData {
    VkCommandBuffer cmd;
    VkSemaphore imageAvailable;
    VkSemaphore renderFinished;
    VkFence inFlight;
};
```

### 2. Resource Management

Centralized GPU resource system:

```cpp
class ResourceSystem {
    GPUBuffer createBuffer(size_t size, VkBufferUsageFlags usage);
    GPUTexture createTexture(const std::string& path);
    DescriptorSet allocateDescriptorSet(PipelineLayout layout);
};
```

### 3. ECS (Entity Component System)

Data-driven entity architecture:

```cpp
// Components
struct Transform { glm::vec3 position; glm::quat rotation; };
struct Crop { uint8_t stage; float water; uint8_t health; };
struct Animal { float hunger; float happiness; bool hasProduct; };

// Systems
class CropSystem : public System {
    void update(float dt) override;
};

class AnimalSystem : public System {
    void update(float dt) override;
};
```

### 4. Chunk-Based World

Optimized for massive farming worlds:

```cpp
struct CropChunk {
    static constexpr int SIZE = 32;
    Crop crops[SIZE][SIZE];
    
    void updateDay();  // Daily growth update
    bool isVisible() const;
};

class WorldStreamer {
    void loadChunksAroundPlayer(glm::vec3 position);
    void unloadDistantChunks();
};
```

### 5. GPU Instancing

Render 100k+ crops in single draw call:

```cpp
struct PlantInstance {
    glm::vec3 position;
    float growthStage;
    float variation;
};

// Render 100,000 plants = 1 draw call
vkCmdDrawIndexed(cmd, indexCount, instanceCount, 0, 0, 0);
```

---

## 🔧 Build Instructions

### Prerequisites

- **Compiler:** GCC 11+ / Clang 13+ / MSVC 2022
- **Vulkan SDK:** 1.3+
- **CMake:** 3.20+
- **Dependencies:** GLFW, GLM, stb_image, tinygltf, Dear ImGui

### Build Steps

```bash
# Clone repository
git clone https://github.com/yourusername/FarmEngine.git
cd FarmEngine

# Create build directory
mkdir build && cd build

# Configure with CMake
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build . --config Release

# Run
./FarmEngine
```

### Dependencies Installation

**Ubuntu/Debian:**
```bash
sudo apt-get install libglfw3-dev libglm-dev libvulkan-dev
```

**Windows (vcpkg):**
```bash
vcpkg install glfw3 glm vulkan stb-image tinygltf imgui
```

---

## 📋 Roadmap

### Phase 1: Foundation (Months 1-3)
- [x] Project structure
- [ ] Vulkan instance & device creation
- [ ] Swapchain & framebuffers
- [ ] Basic triangle rendering
- [ ] Logger & memory system

### Phase 2: 2D Renderer (Months 4-6)
- [ ] Sprite batching system
- [ ] Texture atlas management
- [ ] Tilemap renderer
- [ ] 2D camera system
- [ ] Basic ECS implementation

### Phase 3: 3D Renderer (Months 7-12)
- [ ] Mesh loading (glTF)
- [ ] PBR material system
- [ ] Shadow mapping
- [ ] Terrain renderer
- [ ] LOD system
- [ ] Render Graph implementation

### Phase 4: Simulation (Months 13-18)
- [ ] Crop growth system
- [ ] Season & weather system
- [ ] Animal AI
- [ ] Soil simulation
- [ ] Chunk-based world streaming
- [ ] GPU instancing for vegetation

### Phase 5: Tools & Editor (Months 19-24)
- [ ] Integrated editor (Dear ImGui)
- [ ] Terrain sculpting tools
- [ ] Crop painter
- [ ] Entity inspector
- [ ] Plugin system
- [ ] Modding support

### Phase 6: Optimization (Months 25-30)
- [ ] Multithreading (Job System)
- [ ] Advanced culling (Frustum, Occlusion)
- [ ] Memory optimization
- [ ] Profiling tools
- [ ] VRAM management

---

## 🎮 Example Usage

### Creating a Crop Entity

```cpp
// Create entity
Entity crop = engine.createEntity();

// Add components
crop.addComponent<Transform>(glm::vec3(10.0f, 0.0f, 5.0f));
crop.addComponent<CropSprite>(TextureID::CARROT_STAGE_1);
crop.addComponent<Crop>(CropType::CARROT, 0, 1.0f);

// System will automatically handle growth
```

### Custom Plugin

```cpp
class WeatherPlugin : public Plugin {
public:
    void load() override {
        engine.registerSystem<WeatherSystem>();
    }
    
    void unload() override {
        engine.unregisterSystem<WeatherSystem>();
    }
};

// Register plugin
engine.registerPlugin(std::make_unique<WeatherPlugin>());
```

---

## 📊 Performance Targets

| Feature | Target |
|---------|--------|
| Active Crops | 100,000+ |
| Draw Calls (Frame) | < 500 |
| Frame Time (1080p) | < 8ms |
| World Size | Unlimited (streaming) |
| Memory Usage | < 2GB VRAM |
| Load Time | < 3s |

---

## 🛠️ Technologies Used

- **Rendering:** Vulkan 1.3
- **Windowing:** GLFW / SDL2
- **Math:** GLM
- **Image Loading:** stb_image
- **Model Loading:** tinygltf
- **UI:** Dear ImGui
- **Build System:** CMake
- **Languages:** C++20, GLSL

---

## 📝 License

MIT License - See LICENSE file for details.

---

## 🤝 Contributing

This is an educational/engineering project. Contributions welcome for:
- Vulkan optimizations
- Render Graph improvements
- Simulation algorithms
- Tool development

---

## ⚠️ Reality Check

**Estimated Development Time:** 2-5 years for full feature set
**Lines of Code:** 300,000+ for production-ready engine
**Difficulty:** AAA-level engine engineering

This is not a weekend project. This is serious engine development.

---

## 📚 Learning Resources

- Vulkan Tutorial: https://vulkan-tutorial.com
- Render Graph Architecture: Frostbite, Unreal Engine papers
- ECS Design: Unity DOTS, EnTT documentation
- GPU Instancing: Vulkan best practices

---

**Made with 🧪 by Rick-approved engineering principles**

*"Wubba lubba dub dub! Now go build that engine, Morty!"*
