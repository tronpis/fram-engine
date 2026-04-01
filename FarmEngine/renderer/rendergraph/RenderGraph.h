#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <string>
#include <memory>
#include <functional>
#include <unordered_map>

namespace FarmEngine {

// Forward declarations
class RenderPass;
class Framebuffer;
class Texture;
class Buffer;
class DescriptorSetLayout;

// ============================================================================
// RENDER GRAPH - Node & Resource Types
// ============================================================================

enum class ResourceType {
    Texture2D,
    TextureCube,
    Texture3D,
    Buffer,
    DepthBuffer,
    Transient
};

enum class ResourceAccess {
    None,
    Read,
    Write,
    ReadWrite
};

struct RenderGraphResource {
    std::string name;
    ResourceType type;
    ResourceAccess access;
    
    // Texture resources
    uint32_t width = 0;
    uint32_t height = 0;
    VkFormat format = VK_FORMAT_UNDEFINED;
    VkImageUsageFlags usage = 0;
    
    // External resources
    bool isExternal = false;
    VkImageView externalView = VK_NULL_HANDLE;
    VkImage externalImage = VK_NULL_HANDLE;
};

class RenderGraphNode {
public:
    virtual ~RenderGraphNode() = default;
    
    void setName(const std::string& name) { this->name = name; }
    const std::string& getName() const { return name; }
    
    // Declare resource usage
    void readTexture(const std::string& resourceName);
    void writeTexture(const std::string& resourceName);
    void readBuffer(const std::string& resourceName);
    void writeBuffer(const std::string& resourceName);
    
    // Execution
    virtual void setup(VkCommandBuffer commandBuffer) = 0;
    virtual void execute(VkCommandBuffer commandBuffer) = 0;
    virtual void cleanup() = 0;
    
    // Dependencies
    const std::vector<std::string>& getReadResources() const { return readResources; }
    const std::vector<std::string>& getWriteResources() const { return writeResources; }
    
protected:
    std::string name;
    std::vector<std::string> readResources;
    std::vector<std::string> writeResources;
};

// ============================================================================
// RENDER GRAPH - Pass Implementations
// ============================================================================

struct ShadowPassData {
    VkPipeline pipeline = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
    
    glm::mat4 lightSpaceMatrix;
    glm::vec3 lightPosition;
    float nearPlane = 0.1f;
    float farPlane = 50.0f;
    uint32_t shadowMapSize = 4096;
};

class ShadowPassNode : public RenderGraphNode {
public:
    ShadowPassNode();
    ~ShadowPassNode() override;
    
    bool initialize(VkDevice device, uint32_t shadowMapSize);
    
    void setLightPosition(const glm::vec3& position);
    void setLightSpaceMatrix(const glm::mat4& matrix);
    void bindGeometry(VkBuffer vertexBuffer, VkBuffer indexBuffer, 
                     uint32_t indexCount, VkDescriptorSet materialSet);
    
    void setup(VkCommandBuffer commandBuffer) override;
    void execute(VkCommandBuffer commandBuffer) override;
    void cleanup() override;
    
    const ShadowPassData& getData() const { return data; }
    
private:
    VkDevice device = VK_NULL_HANDLE;
    ShadowPassData data;
    
    struct DrawCall {
        VkBuffer vertexBuffer;
        VkBuffer indexBuffer;
        uint32_t indexCount;
        VkDescriptorSet materialSet;
    };
    std::vector<DrawCall> drawCalls;
};

struct GBufferPassData {
    VkPipeline pipeline = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    
    // G-Buffer textures (created by render graph)
    std::string albedoTarget;
    std::string normalTarget;
    std::string pbrTarget;      // metallic, roughness, ao
    std::string depthTarget;
};

class GBufferPassNode : public RenderGraphNode {
public:
    GBufferPassNode();
    ~GBufferPassNode() override;
    
    bool initialize(VkDevice device, uint32_t width, uint32_t height);
    
    void setViewProjection(const glm::mat4& view, const glm::mat4& projection);
    void setCameraPosition(const glm::vec3& position);
    
    void submitMesh(VkBuffer vertexBuffer, VkBuffer indexBuffer,
                   uint32_t indexCount, const glm::mat4& transform,
                   VkDescriptorSet materialSet);
    
    void setup(VkCommandBuffer commandBuffer) override;
    void execute(VkCommandBuffer commandBuffer) override;
    void cleanup() override;
    
private:
    VkDevice device = VK_NULL_HANDLE;
    GBufferPassData data;
    
    glm::mat4 viewMatrix;
    glm::mat4 projectionMatrix;
    glm::vec3 cameraPosition;
    
    struct MeshDraw {
        VkBuffer vertexBuffer;
        VkBuffer indexBuffer;
        uint32_t indexCount;
        glm::mat4 transform;
        VkDescriptorSet materialSet;
    };
    std::vector<MeshDraw> meshDraws;
};

struct LightingPassData {
    VkPipeline pipeline = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
    
    // Light uniforms
    glm::vec3 ambientLight;
    int32_t pointLightCount = 0;
    int32_t directionalLightCount = 1;
    int32_t spotLightCount = 0;
};

struct PointLight {
    glm::vec4 position;     // w = radius
    glm::vec4 color;        // w = intensity
    glm::vec4 direction;    // for spot lights
    float innerAngle;
    float outerAngle;
    int32_t castsShadow;
    int32_t padding[2];
};

class LightingPassNode : public RenderGraphNode {
public:
    LightingPassNode();
    ~LightingPassNode() override;
    
    bool initialize(VkDevice device, uint32_t width, uint32_t height);
    
    void setAmbientLight(const glm::vec3& ambient);
    void addPointLight(const PointLight& light);
    void setDirectionalLight(int32_t index, const PointLight& light);
    
    void setup(VkCommandBuffer commandBuffer) override;
    void execute(VkCommandBuffer commandBuffer) override;
    void cleanup() override;
    
private:
    VkDevice device = VK_NULL_HANDLE;
    LightingPassData data;
    
    std::vector<PointLight> pointLights;
    std::vector<PointLight> directionalLights;
};

struct TransparentPassData {
    VkPipeline pipeline = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    
    // Sorted transparent objects
    struct TransparentObject {
        VkBuffer vertexBuffer;
        VkBuffer indexBuffer;
        uint32_t indexCount;
        glm::mat4 transform;
        VkDescriptorSet materialSet;
        float distanceToCamera;
    };
};

class TransparentPassNode : public RenderGraphNode {
public:
    TransparentPassNode();
    ~TransparentPassNode() override;
    
    bool initialize(VkDevice device, uint32_t width, uint32_t height);
    
    void setViewProjection(const glm::mat4& view, const glm::mat4& projection);
    void setCameraPosition(const glm::vec3& position);
    
    void submitTransparent(const TransparentPassData::TransparentObject& obj);
    
    void setup(VkCommandBuffer commandBuffer) override;
    void execute(VkCommandBuffer commandBuffer) override;
    void cleanup() override;
    
private:
    void sortTransparentObjects();
    
    VkDevice device = VK_NULL_HANDLE;
    TransparentPassData data;
    
    glm::mat4 viewMatrix;
    glm::mat4 projectionMatrix;
    glm::vec3 cameraPosition;
    
    std::vector<TransparentPassData::TransparentObject> transparentObjects;
};

struct PostProcessPassData {
    VkPipeline pipeline = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
    
    // Post effects
    bool bloomEnabled = true;
    bool tonemappingEnabled = true;
    bool vignetteEnabled = true;
    bool chromaticAberrationEnabled = false;
    
    float exposure = 1.0f;
    float gamma = 2.2f;
    float bloomThreshold = 0.8f;
    float bloomIntensity = 0.5f;
    float vignetteIntensity = 0.3f;
};

class PostProcessPassNode : public RenderGraphNode {
public:
    PostProcessPassNode();
    ~PostProcessPassNode() override;
    
    bool initialize(VkDevice device, uint32_t width, uint32_t height);
    
    void configure(bool bloom, bool tonemap, bool vignette, bool ca);
    void setExposure(float exp);
    void setGamma(float g);
    
    void setup(VkCommandBuffer commandBuffer) override;
    void execute(VkCommandBuffer commandBuffer) override;
    void cleanup() override;
    
private:
    VkDevice device = VK_NULL_HANDLE;
    PostProcessPassData data;
};

struct UIPassData {
    VkPipeline pipeline = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
    
    struct UIMesh {
        VkBuffer vertexBuffer;
        VkBuffer indexBuffer;
        uint32_t indexCount;
        VkDescriptorSet textureSet;
    };
};

class UIPassNode : public RenderGraphNode {
public:
    UIPassNode();
    ~UIPassNode() override;
    
    bool initialize(VkDevice device, uint32_t width, uint32_t height);
    
    void submitUI(const UIPassData::UIMesh& mesh);
    
    void setup(VkCommandBuffer commandBuffer) override;
    void execute(VkCommandBuffer commandBuffer) override;
    void cleanup() override;
    
private:
    VkDevice device = VK_NULL_HANDLE;
    UIPassData data;
    
    std::vector<UIPassData::UIMesh> uiMeshes;
};

// ============================================================================
// RENDER GRAPH - Main Graph
// ============================================================================

class RenderGraph {
public:
    RenderGraph();
    ~RenderGraph();
    
    bool initialize(VkDevice device, VkPhysicalDevice physicalDevice,
                   uint32_t width, uint32_t height);
    void cleanup();
    
    // Resource management
    void createTexture(const std::string& name, uint32_t width, uint32_t height,
                      VkFormat format, VkImageUsageFlags usage);
    void createBuffer(const std::string& name, VkDeviceSize size,
                     VkBufferUsageFlags usage);
    void importExternalTexture(const std::string& name, VkImageView view, 
                              VkImage image, uint32_t width, uint32_t height);
    
    // Pass registration
    ShadowPassNode* addShadowPass(const std::string& name = "ShadowPass");
    GBufferPassNode* addGBufferPass(const std::string& name = "GBufferPass");
    LightingPassNode* addLightingPass(const std::string& name = "LightingPass");
    TransparentPassNode* addTransparentPass(const std::string& name = "TransparentPass");
    PostProcessPassNode* addPostProcessPass(const std::string& name = "PostProcessPass");
    UIPassNode* addUIPass(const std::string& name = "UIPass");
    
    void addCustomPass(std::unique_ptr<RenderGraphNode> node);
    
    // Compilation & execution
    void compile();
    void execute(VkCommandBuffer commandBuffer);
    
    // Resize
    void resize(uint32_t newWidth, uint32_t newHeight);
    
    // Access final output
    VkImageView getOutputView() const { return outputView; }
    VkImage getOutputImage() const { return outputImage; }
    
private:
    void buildDependencyGraph();
    void schedulePasses();
    void createTransientResources();
    void destroyTransientResources();
    
    VkDevice device = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    
    uint32_t width = 0;
    uint32_t height = 0;
    
    std::unordered_map<std::string, RenderGraphResource> resources;
    std::vector<std::unique_ptr<RenderGraphNode>> nodes;
    
    // Execution order (computed during compile)
    std::vector<RenderGraphNode*> executionOrder;
    
    // Final output
    VkImageView outputView = VK_NULL_HANDLE;
    VkImage outputImage = VK_NULL_HANDLE;
    
    bool compiled = false;
};

} // namespace FarmEngine
