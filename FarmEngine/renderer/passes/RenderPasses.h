#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <string>
#include <functional>

namespace FarmEngine {

// Tipos de render passes
enum class RenderPassType {
    Shadow,
    GBuffer,
    Lighting,
    Transparent,
    UI,
    PostProcess
};

struct RenderPassConfig {
    RenderPassType type;
    std::string name;
    uint32_t width;
    uint32_t height;
    std::vector<VkAttachmentDescription> attachments;
    std::vector<VkSubpassDescription> subpasses;
    std::vector<VkSubpassDependency> dependencies;
};

class RenderPass {
public:
    RenderPass(VkDevice device);
    ~RenderPass();
    
    bool initialize(const RenderPassConfig& config);
    VkRenderPass getHandle() const { return renderPass; }
    const RenderPassConfig& getConfig() const { return config; }
    
private:
    VkDevice device;
    VkRenderPass renderPass = VK_NULL_HANDLE;
    RenderPassConfig config;
};

class Framebuffer {
public:
    Framebuffer(VkDevice device, VkRenderPass renderPass);
    ~Framebuffer();
    
    bool initialize(uint32_t width, uint32_t height, 
                   const std::vector<VkImageView>& attachments);
    
    VkFramebuffer getHandle() const { return framebuffer; }
    uint32_t getWidth() const { return width; }
    uint32_t getHeight() const { return height; }
    
    void resize(uint32_t newWidth, uint32_t newHeight);
    
private:
    VkDevice device;
    VkRenderPass renderPass;
    VkFramebuffer framebuffer = VK_NULL_HANDLE;
    uint32_t width;
    uint32_t height;
};

// Attachment helpers
class AttachmentBuilder {
public:
    static VkAttachmentDescription colorAttachment(
        VkFormat format,
        VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT,
        VkAttachmentLoadOp loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        VkAttachmentStoreOp storeOp = VK_ATTACHMENT_STORE_OP_STORE);
    
    static VkAttachmentDescription depthAttachment(
        VkFormat format,
        VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT,
        VkAttachmentLoadOp loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        VkAttachmentStoreOp storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE);
    
    static VkAttachmentReference colorReference(uint32_t attachment, 
                                                VkImageLayout layout);
    static VkAttachmentReference depthReference(uint32_t attachment,
                                                VkImageLayout layout);
};

// Render Pass Factory
class RenderPassFactory {
public:
    static RenderPassConfig createShadowPass(uint32_t size);
    static RenderPassConfig createGBufferPass(uint32_t width, uint32_t height);
    static RenderPassConfig createLightingPass(uint32_t width, uint32_t height);
    static RenderPassConfig createTransparentPass(uint32_t width, uint32_t height);
    static RenderPassConfig createUIPass(uint32_t width, uint32_t height);
    static RenderPassConfig createPostProcessPass(uint32_t width, uint32_t height);
};

// Manager de render passes
class RenderPassManager {
public:
    static RenderPassManager& getInstance();
    
    void initialize(VkDevice device, uint32_t screenWidth, uint32_t screenHeight);
    void cleanup();
    
    RenderPass* getRenderPass(RenderPassType type);
    Framebuffer* getFramebuffer(RenderPassType type);
    
    void recreateFramebuffers(uint32_t newWidth, uint32_t newHeight);
    
private:
    RenderPassManager() = default;
    ~RenderPassManager();
    
    VkDevice device = VK_NULL_HANDLE;
    uint32_t screenWidth;
    uint32_t screenHeight;
    
    std::vector<std::unique_ptr<RenderPass>> renderPasses;
    std::vector<std::unique_ptr<Framebuffer>> framebuffers;
};

} // namespace FarmEngine
