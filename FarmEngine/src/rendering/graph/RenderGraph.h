#pragma once

#include <vulkan/vulkan.h>
#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <unordered_map>
#include <set>

namespace FarmEngine::Render {

// Forward declarations
class RenderGraph;
class ResourceRegistry;

enum class ResourceType {
    ColorAttachment,
    DepthAttachment,
    InputAttachment,
    StorageImage,
    UniformBuffer,
    VertexBuffer,
    IndexBuffer
};

enum class ResourceLifetime {
    Frame,      // Se crea/destruye cada frame (ej: GBuffer temporal)
    Persistent, // Vive mientras viva el graph (ej: Swapchain, Texturas cargadas)
    External    // Gestionado externamente (ej: Swapchain images provistas por el engine)
};

struct ResourceHandle {
    std::string name;
    ResourceType type;
    ResourceLifetime lifetime;
    
    VkImage image = VK_NULL_HANDLE;
    VkImageView imageView = VK_NULL_HANDLE;
    VkFormat format = VK_FORMAT_UNDEFINED;
    VkExtent2D extent = {0, 0};
    uint32_t arrayLayers = 1;
    uint32_t mipLevels = 1;
    
    // Estado actual
    VkImageLayout currentLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    VkAccessFlags currentAccess = 0;
    VkPipelineStageFlags currentStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    
    bool isSwapchain = false;
};

struct PassDependency {
    uint32_t from;
    uint32_t to;
    VkPipelineStageFlags srcStageMask;
    VkPipelineStageFlags dstStageMask;
    VkAccessFlags srcAccessMask;
    VkAccessFlags dstAccessMask;
};

class RenderPass {
public:
    using ExecuteFunc = std::function<void(VkCommandBuffer, const ResourceRegistry&)>;
    
    std::string name;
    std::vector<std::string> colorAttachments;
    std::string depthAttachment;
    std::vector<std::string> inputAttachments;
    std::vector<std::string> storageAttachments;
    
    ExecuteFunc executeCallback;
    
    // Configuración de clear/load/store
    bool clearColor = false;
    bool clearDepth = false;
    VkClearColorValue clearColorValue = {{0.0f, 0.0f, 0.0f, 1.0f}};
    VkClearDepthStencilValue clearDepthValue = {1.0f, 0};
    
    VkAttachmentLoadOp colorLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    VkAttachmentStoreOp colorStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
    VkAttachmentLoadOp depthLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    VkAttachmentStoreOp depthStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
};

class RenderGraphBuilder {
public:
    RenderGraphBuilder() = default;
    
    // Definir recursos
    RenderGraphBuilder& addColorTarget(const std::string& name, VkFormat format, VkExtent2D extent, ResourceLifetime lifetime = ResourceLifetime::Frame);
    RenderGraphBuilder& addDepthTarget(const std::string& name, VkFormat format, VkExtent2D extent, ResourceLifetime lifetime = ResourceLifetime::Frame);
    RenderGraphBuilder& addExternalImage(const std::string& name, VkImage image, VkImageView view, VkFormat format, VkExtent2D extent);
    
    // Definir passes
    RenderGraphBuilder& addPass(const std::string& name, 
                                const std::function<void(RenderPass&)>& configureFunc);
    
    // Definir dependencias explícitas (opcional, se pueden inferir)
    RenderGraphBuilder& addDependency(uint32_t from, uint32_t to, 
                                      VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage,
                                      VkAccessFlags srcAccess, VkAccessFlags dstAccess);

private:
    friend class RenderGraph;
    std::vector<ResourceHandle> resources;
    std::vector<RenderPass> passes;
    std::vector<PassDependency> explicitDependencies;
};

class ResourceRegistry {
public:
    const ResourceHandle* getResource(const std::string& name) const;
    VkImageView getImageView(const std::string& name) const;
    VkImage getImage(const std::string& name) const;
    VkFormat getFormat(const std::string& name) const;
    
    void updateResourceState(const std::string& name, 
                             VkImageLayout newLayout, 
                             VkAccessFlags newAccess, 
                             VkPipelineStageFlags newStage);
    
private:
    friend class RenderGraph;
    std::unordered_map<std::string, size_t> nameToIndex;
    std::vector<ResourceHandle> resources;
};

class RenderGraph {
public:
    RenderGraph() = default;
    ~RenderGraph();
    
    void compile(RenderGraphBuilder&& builder);
    void execute(VkCommandBuffer cmd, ResourceRegistry& registry, uint32_t frameIndex);
    
    // Acceso a recursos compilados
    const ResourceRegistry& getRegistry() const { return compiledRegistry; }
    
    void cleanup(VkDevice device);

private:
    struct CompiledPass {
        RenderPass definition;
        std::vector<VkAttachmentDescription> attachments;
        std::vector<VkAttachmentReference> colorRefs;
        VkAttachmentReference depthRef = {VK_ATTACHMENT_UNUSED, VK_IMAGE_LAYOUT_UNDEFINED};
        std::vector<VkSubpassDependency> dependencies;
        VkRenderPass vkRenderPass = VK_NULL_HANDLE;
        VkFramebuffer framebuffer = VK_NULL_HANDLE;
        
        // Extent calculado durante la creación del framebuffer para usar en execute()
        VkExtent2D extent = {0, 0};
        
        // Barreras de transición antes del pass
        std::vector<VkImageMemoryBarrier> prePassBarriers;
    };
    
    std::vector<CompiledPass> compiledPasses;
    ResourceRegistry compiledRegistry;
    std::vector<ResourceHandle> persistentResources;
    
    VkDevice device = VK_NULL_HANDLE;
    
    void createRenderPasses(VkDevice dev);
    void createFramebuffers(VkDevice dev, VkExtent2D swapchainExtent);
    void recordBarriers(VkCommandBuffer cmd, const CompiledPass& pass);
};

} // namespace FarmEngine::Render
