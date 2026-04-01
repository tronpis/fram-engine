#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <string>
#include <memory>
#include <unordered_map>
#include <functional>

namespace FarmEngine {

// ============================================================================
// PIPELINE SYSTEM - Shader Compiler (SPIR-V)
// ============================================================================

enum class ShaderStage {
    Vertex,
    Fragment,
    Compute,
    Geometry,
    TessellationControl,
    TessellationEvaluation
};

struct ShaderSource {
    std::string source;
    ShaderStage stage;
    std::string entryPoint = "main";
    std::vector<std::pair<std::string, std::string>> defines;
};

class ShaderModule {
public:
    ShaderModule(VkDevice device);
    ~ShaderModule();
    
    bool compileFromSource(const ShaderSource& source);
    bool loadFromFile(const std::string& path);
    bool loadFromSPIRV(const uint32_t* spirv, size_t size);
    void cleanup();
    
    VkShaderModule getHandle() const { return module; }
    VkShaderStageFlagBits getStage() const { return stage; }
    const std::string& getEntryPoint() const { return entryPoint; }
    
private:
    bool compileGLSLToSPIRV(const std::string& glsl, ShaderStage stage);
    
    VkDevice device;
    VkShaderModule module = VK_NULL_HANDLE;
    VkShaderStageFlagBits stage;
    std::string entryPoint;
    std::vector<uint32_t> spirvCode;
};

class ShaderCompiler {
public:
    static ShaderCompiler& getInstance();
    
    bool initialize();
    void cleanup();
    
    std::unique_ptr<ShaderModule> createShader(const ShaderSource& source);
    std::unique_ptr<ShaderModule> loadShader(const std::string& path);
    
    // Hot reload
    bool reloadShader(ShaderModule* module);
    void watchDirectory(const std::string& path);
    
private:
    ShaderCompiler() = default;
    ~ShaderCompiler();
    
    std::unordered_map<std::string, std::unique_ptr<ShaderModule>> shaderCache;
    bool initialized = false;
};

// ============================================================================
// PIPELINE SYSTEM - Pipeline Layouts
// ============================================================================

struct PushConstantRange {
    VkShaderStageFlags stages;
    uint32_t offset;
    uint32_t size;
};

struct PipelineLayoutConfig {
    std::vector<class DescriptorSetLayout*> descriptorSetLayouts;
    std::vector<PushConstantRange> pushConstantRanges;
};

class PipelineLayout {
public:
    PipelineLayout(VkDevice device);
    ~PipelineLayout();
    
    bool initialize(const PipelineLayoutConfig& config);
    void cleanup();
    
    VkPipelineLayout getHandle() const { return layout; }
    const PipelineLayoutConfig& getConfig() const { return config; }
    
private:
    VkDevice device;
    VkPipelineLayout layout = VK_NULL_HANDLE;
    PipelineLayoutConfig config;
};

// ============================================================================
// PIPELINE SYSTEM - Pipeline Cache & Builder
// ============================================================================

enum class PrimitiveTopology {
    TriangleList,
    TriangleStrip,
    LineList,
    LineStrip,
    PointList
};

enum class PolygonMode {
    Fill,
    Line,
    Point
};

enum class CullMode {
    None,
    Front,
    Back,
    FrontAndBack
};

enum class FrontFace {
    Clockwise,
    CounterClockwise
};

struct RasterizationState {
    PolygonMode polygonMode = PolygonMode::Fill;
    CullMode cullMode = CullMode::Back;
    FrontFace frontFace = FrontFace::CounterClockwise;
    bool depthClampEnable = false;
    bool rasterizerDiscardEnable = false;
    float lineWidth = 1.0f;
};

struct ColorBlendAttachment {
    bool blendEnable = false;
    VkBlendFactor srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    VkBlendFactor dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    VkBlendOp colorBlendOp = VK_BLEND_OP_ADD;
    VkBlendFactor srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    VkBlendFactor dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    VkBlendOp alphaBlendOp = VK_BLEND_OP_ADD;
    VkColorComponentFlags colorWriteMask = VK_COLOR_COMPONENT_R_BIT | 
                                           VK_COLOR_COMPONENT_G_BIT | 
                                           VK_COLOR_COMPONENT_B_BIT | 
                                           VK_COLOR_COMPONENT_A_BIT;
};

struct DepthStencilState {
    bool depthTestEnable = true;
    bool depthWriteEnable = true;
    VkCompareOp depthCompareOp = VK_COMPARE_OP_LESS;
    bool stencilTestEnable = false;
    VkStencilOp frontFailOp = VK_STENCIL_OP_KEEP;
    VkStencilOp frontPassOp = VK_STENCIL_OP_KEEP;
    VkStencilOp frontDepthFailOp = VK_STENCIL_OP_KEEP;
    VkCompareOp frontCompareOp = VK_COMPARE_OP_ALWAYS;
    uint32_t frontCompareMask = 0xFF;
    uint32_t frontWriteMask = 0xFF;
    uint32_t frontReference = 0;
};

struct MultisampleState {
    VkSampleCountFlagBits rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    float sampleShadingEnable = false;
    float minSampleShading = 1.0f;
    const VkSampleMask* sampleMask = nullptr;
    bool alphaToCoverageEnable = false;
    bool alphaToOneEnable = false;
};

struct PipelineConfig {
    std::string name;
    
    // Shaders
    std::vector<ShaderModule*> shaders;
    
    // Vertex input
    std::vector<VkVertexInputBindingDescription> bindingDescriptions;
    std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
    
    // Input assembly
    PrimitiveTopology topology = PrimitiveTopology::TriangleList;
    bool primitiveRestartEnable = false;
    
    // Viewport state (dynamic)
    uint32_t viewportCount = 1;
    uint32_t scissorCount = 1;
    
    // Rasterization
    RasterizationState rasterization;
    
    // Multisampling
    MultisampleState multisample;
    
    // Depth/Stencil
    DepthStencilState depthStencil;
    
    // Color blending
    std::vector<ColorBlendAttachment> colorBlendAttachments;
    VkColorComponentFlags colorBlendFlags = VK_COLOR_COMPONENT_R_BIT | 
                                            VK_COLOR_COMPONENT_G_BIT | 
                                            VK_COLOR_COMPONENT_B_BIT | 
                                            VK_COLOR_COMPONENT_A_BIT;
    
    // Dynamic states
    std::vector<VkDynamicState> dynamicStates;
    
    // Pipeline layout
    PipelineLayout* layout = nullptr;
    
    // Render pass
    VkRenderPass renderPass = VK_NULL_HANDLE;
    uint32_t subpass = 0;
    
    // Base pipeline (for derivates)
    VkPipeline basePipeline = VK_NULL_HANDLE;
    int32_t basePipelineIndex = -1;
};

class GraphicsPipeline {
public:
    GraphicsPipeline(VkDevice device, VkPipelineCache cache);
    ~GraphicsPipeline();
    
    bool initialize(const PipelineConfig& config);
    void cleanup();
    
    VkPipeline getHandle() const { return pipeline; }
    const PipelineConfig& getConfig() const { return config; }
    
private:
    VkDevice device;
    VkPipelineCache cache;
    VkPipeline pipeline = VK_NULL_HANDLE;
    PipelineConfig config;
};

class ComputePipeline {
public:
    ComputePipeline(VkDevice device, VkPipelineCache cache);
    ~ComputePipeline();
    
    bool initialize(ShaderModule* shader, PipelineLayout* layout);
    void cleanup();
    
    VkPipeline getHandle() const { return pipeline; }
    
private:
    VkDevice device;
    VkPipelineCache cache;
    VkPipeline pipeline = VK_NULL_HANDLE;
};

class PipelineCache {
public:
    PipelineCache(VkDevice device, VkPhysicalDevice physicalDevice);
    ~PipelineCache();
    
    bool initialize();
    void cleanup();
    
    bool loadFromFile(const std::string& path);
    bool saveToFile(const std::string& path);
    
    VkPipelineCache getHandle() const { return cache; }
    
    void reset();
    
private:
    VkDevice device;
    VkPhysicalDevice physicalDevice;
    VkPipelineCache cache = VK_NULL_HANDLE;
};

// ============================================================================
// PIPELINE SYSTEM - Pipeline Manager
// ============================================================================

class PipelineSystem {
public:
    static PipelineSystem& getInstance();
    
    bool initialize(VkDevice device, VkPhysicalDevice physicalDevice);
    void cleanup();
    
    // Creation
    GraphicsPipeline* createGraphicsPipeline(const PipelineConfig& config);
    ComputePipeline* createComputePipeline(ShaderModule* shader, PipelineLayout* layout);
    PipelineLayout* createPipelineLayout(const PipelineLayoutConfig& config);
    
    // Loading
    ShaderModule* loadShader(const std::string& path);
    
    // Cache management
    void saveCache(const std::string& path);
    void loadCache(const std::string& path);
    
    // Accessors
    PipelineCache* getCache() { return cache.get(); }
    ShaderCompiler& getShaderCompiler() { return ShaderCompiler::getInstance(); }
    
    size_t getPipelineCount() const { return pipelines.size(); }
    
private:
    PipelineSystem() = default;
    ~PipelineSystem();
    
    VkDevice device = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    
    std::unique_ptr<PipelineCache> cache;
    std::vector<std::unique_ptr<GraphicsPipeline>> pipelines;
    std::vector<std::unique_ptr<ComputePipeline>> computePipelines;
    std::vector<std::unique_ptr<PipelineLayout>> layouts;
    std::unordered_map<std::string, std::unique_ptr<ShaderModule>> shaders;
};

} // namespace FarmEngine
