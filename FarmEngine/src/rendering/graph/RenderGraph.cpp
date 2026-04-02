#include "RenderGraph.h"
#include <algorithm>
#include <stdexcept>

namespace FarmEngine::Render {

// ============================================================================
// ResourceRegistry Implementation
// ============================================================================

const ResourceHandle* ResourceRegistry::getResource(const std::string& name) const {
    auto it = nameToIndex.find(name);
    if (it == nameToIndex.end()) {
        return nullptr;
    }
    return &resources[it->second];
}

VkImageView ResourceRegistry::getImageView(const std::string& name) const {
    const ResourceHandle* res = getResource(name);
    return res ? res->imageView : VK_NULL_HANDLE;
}

VkImage ResourceRegistry::getImage(const std::string& name) const {
    const ResourceHandle* res = getResource(name);
    return res ? res->image : VK_NULL_HANDLE;
}

VkFormat ResourceRegistry::getFormat(const std::string& name) const {
    const ResourceHandle* res = getResource(name);
    return res ? res->format : VK_FORMAT_UNDEFINED;
}

void ResourceRegistry::updateResourceState(const std::string& name,
                                           VkImageLayout newLayout,
                                           VkAccessFlags newAccess,
                                           VkPipelineStageFlags newStage) {
    auto it = nameToIndex.find(name);
    if (it != nameToIndex.end()) {
        resources[it->second].currentLayout = newLayout;
        resources[it->second].currentAccess = newAccess;
        resources[it->second].currentStage = newStage;
    }
}

// ============================================================================
// RenderGraphBuilder Implementation
// ============================================================================

RenderGraphBuilder& RenderGraphBuilder::addColorTarget(const std::string& name, 
                                                        VkFormat format, 
                                                        VkExtent2D extent, 
                                                        ResourceLifetime lifetime) {
    ResourceHandle handle;
    handle.name = name;
    handle.type = ResourceType::ColorAttachment;
    handle.lifetime = lifetime;
    handle.format = format;
    handle.extent = extent;
    handle.currentLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    resources.push_back(handle);
    return *this;
}

RenderGraphBuilder& RenderGraphBuilder::addDepthTarget(const std::string& name,
                                                        VkFormat format,
                                                        VkExtent2D extent,
                                                        ResourceLifetime lifetime) {
    ResourceHandle handle;
    handle.name = name;
    handle.type = ResourceType::DepthAttachment;
    handle.lifetime = lifetime;
    handle.format = format;
    handle.extent = extent;
    handle.currentLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    resources.push_back(handle);
    return *this;
}

RenderGraphBuilder& RenderGraphBuilder::addExternalImage(const std::string& name,
                                                          VkImage image,
                                                          VkImageView view,
                                                          VkFormat format,
                                                          VkExtent2D extent) {
    ResourceHandle handle;
    handle.name = name;
    handle.type = ResourceType::ColorAttachment;
    handle.lifetime = ResourceLifetime::External;
    handle.image = image;
    handle.imageView = view;
    handle.format = format;
    handle.extent = extent;
    handle.isSwapchain = true;
    resources.push_back(handle);
    return *this;
}

RenderGraphBuilder& RenderGraphBuilder::addPass(const std::string& name,
                                                 const std::function<void(RenderPass&)>& configureFunc) {
    RenderPass pass;
    pass.name = name;
    configureFunc(pass);
    passes.push_back(pass);
    return *this;
}

RenderGraphBuilder& RenderGraphBuilder::addDependency(uint32_t from, uint32_t to,
                                                       VkPipelineStageFlags srcStage,
                                                       VkPipelineStageFlags dstStage,
                                                       VkAccessFlags srcAccess,
                                                       VkAccessFlags dstAccess) {
    PassDependency dep;
    dep.from = from;
    dep.to = to;
    dep.srcStageMask = srcStage;
    dep.dstStageMask = dstStage;
    dep.srcAccessMask = srcAccess;
    dep.dstAccessMask = dstAccess;
    explicitDependencies.push_back(dep);
    return *this;
}

// ============================================================================
// RenderGraph Implementation
// ============================================================================
    // Copiar recursos y passes
    compiledRegistry.resources = std::move(builder.resources);
    
    // Limpiar estado previo antes de compilar
    compiledPasses.clear();
    
    compiledPasses.resize(builder.passes.size());

    
    // Construir mapa nombre->índice
    for (size_t i = 0; i < compiledRegistry.resources.size(); ++i) {
        compiledRegistry.nameToIndex[compiledRegistry.resources[i].name] = i;
    }
    
    // Compilar cada pass
    for (size_t i = 0; i < builder.passes.size(); ++i) {
        CompiledPass& compiled = compiledPasses[i];
        compiled.definition = builder.passes[i];
        
        // Resolver attachments
        uint32_t attachmentIndex = 0;
        
        // Color attachments
        for (const auto& colorName : compiled.definition.colorAttachments) {
            const ResourceHandle* res = compiledRegistry.getResource(colorName);
            if (!res) {
                throw std::runtime_error("Color attachment not found: " + colorName);
            }
            
            VkAttachmentDescription desc{};
            desc.format = res->format;
            desc.samples = VK_SAMPLE_COUNT_1_BIT;
            desc.loadOp = compiled.definition.colorLoadOp;
            desc.storeOp = compiled.definition.colorStoreOp;
            desc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            desc.initialLayout = res->currentLayout;
            desc.finalLayout = res->currentLayout; // Se actualizará con barreras
            
            compiled.attachments.push_back(desc);
            
            VkAttachmentReference ref{};
            ref.attachment = attachmentIndex++;
            ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            compiled.colorRefs.push_back(ref);
        }
        
        // Depth attachment
        if (!compiled.definition.depthAttachment.empty()) {
            const ResourceHandle* res = compiledRegistry.getResource(compiled.definition.depthAttachment);
            if (!res) {
                throw std::runtime_error("Depth attachment not found: " + compiled.definition.depthAttachment);
            }
            
            VkAttachmentDescription desc{};
            desc.format = res->format;
            desc.samples = VK_SAMPLE_COUNT_1_BIT;
            desc.loadOp = compiled.definition.depthLoadOp;
            desc.storeOp = compiled.definition.depthStoreOp;
            desc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            desc.initialLayout = res->currentLayout;
            desc.finalLayout = res->currentLayout;
            
            compiled.attachments.push_back(desc);
            
            compiled.depthRef.attachment = attachmentIndex++;
            compiled.depthRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        }
        
        // Dependencias automáticas basadas en uso de recursos
        // TODO: Implementar análisis de dependencias del grafo
    }
    
    // Las dependencias entre passes separados se manejan mediante barreras de pipeline
    // registradas durante la ejecución, no mediante VkSubpassDependency
    // VkSubpassDependency solo es válido para subpasses dentro del mismo VkRenderPass
}

void RenderGraph::createRenderPasses(VkDevice dev) {
    device = dev;
    
    for (auto& compiled : compiledPasses) {
        VkRenderPassCreateInfo rpInfo{};
        rpInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        rpInfo.attachmentCount = static_cast<uint32_t>(compiled.attachments.size());
        rpInfo.pAttachments = compiled.attachments.data();
        rpInfo.subpassCount = 1;
        
        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = static_cast<uint32_t>(compiled.colorRefs.size());
        subpass.pColorAttachments = compiled.colorRefs.data();
        subpass.pDepthStencilAttachment = compiled.depthRef.attachment != VK_ATTACHMENT_UNUSED ? &compiled.depthRef : nullptr;
        
        rpInfo.pSubpasses = &subpass;
        rpInfo.dependencyCount = static_cast<uint32_t>(compiled.dependencies.size());
        rpInfo.pDependencies = compiled.dependencies.data();
        
        if (vkCreateRenderPass(device, &rpInfo, nullptr, &compiled.vkRenderPass) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create render pass: " + compiled.definition.name);
        }
    }
}

void RenderGraph::createFramebuffers(VkDevice dev, VkExtent2D swapchainExtent) {
    for (auto& compiled : compiledPasses) {
        std::vector<VkImageView> attachments;
        VkExtent2D extent = swapchainExtent;
        
        // Obtener views de todos los attachments
        for (const auto& colorName : compiled.definition.colorAttachments) {
            const ResourceHandle* res = compiledRegistry.getResource(colorName);
            if (!res) {
                throw std::runtime_error("Color attachment not found: " + colorName);
            }
            if (res->imageView == VK_NULL_HANDLE) {
                throw std::runtime_error("Color attachment has null imageView: " + colorName + 
                                         ". For external targets, use addExternalImage() instead of addColorTarget().");
            }
            attachments.push_back(res->imageView);
            if (!res->isSwapchain) {
                extent = res->extent;
            }
        }
        
        if (!compiled.definition.depthAttachment.empty()) {
            const ResourceHandle* res = compiledRegistry.getResource(compiled.definition.depthAttachment);
            if (!res) {
                throw std::runtime_error("Depth attachment not found: " + compiled.definition.depthAttachment);
            }
            if (res->imageView == VK_NULL_HANDLE) {
                throw std::runtime_error("Depth attachment has null imageView: " + compiled.definition.depthAttachment);
            }
            attachments.push_back(res->imageView);
        }
        
        // Guardar el extent calculado en el CompiledPass para usarlo en execute()
        compiled.extent = extent;
        
        VkFramebufferCreateInfo fbInfo{};
        fbInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        fbInfo.renderPass = compiled.vkRenderPass;
        fbInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        fbInfo.pAttachments = attachments.data();
        fbInfo.width = extent.width;
        fbInfo.height = extent.height;
        fbInfo.layers = 1;
        
        if (vkCreateFramebuffer(device, &fbInfo, nullptr, &compiled.framebuffer) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create framebuffer: " + compiled.definition.name);
        }
    }
}

void RenderGraph::recordBarriers(VkCommandBuffer cmd, const CompiledPass& pass) {
    // Ejecutar barreras pre-pass si existen
    if (!pass.prePassBarriers.empty()) {
        vkCmdPipelineBarrier(
            cmd,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            0,
            0, nullptr,
            0, nullptr,
            static_cast<uint32_t>(pass.prePassBarriers.size()),
            pass.prePassBarriers.data()
        );
    }
}

void RenderGraph::execute(VkCommandBuffer cmd, ResourceRegistry& registry, uint32_t frameIndex) {
    if (compiledPasses.empty()) {
        return;
    }
    
    for (const auto& compiled : compiledPasses) {
        // 1. Record barreras de transición
        recordBarriers(cmd, compiled);
        
        // 2. Begin render pass
        VkRenderPassBeginInfo rpBegin{};
        rpBegin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        rpBegin.renderPass = compiled.vkRenderPass;
        rpBegin.framebuffer = compiled.framebuffer;
        rpBegin.renderArea.extent = compiled.extent;
        
        // Clear values - uno por cada attachment (color attachments primero, luego depth)
        std::vector<VkClearValue> clearValues;
        clearValues.reserve(compiled.attachments.size());
        
        // Color attachments
        for (size_t i = 0; i < compiled.definition.colorAttachments.size(); ++i) {
            VkClearValue colorClear{};
            colorClear.color = compiled.definition.clearColorValue;
            clearValues.push_back(colorClear);
        }
        
        // Depth attachment
        if (!compiled.definition.depthAttachment.empty()) {
            VkClearValue depthClear{};
            depthClear.depthStencil = compiled.definition.clearDepthValue;
            clearValues.push_back(depthClear);
        }
        
        rpBegin.clearValueCount = static_cast<uint32_t>(clearValues.size());
        rpBegin.pClearValues = clearValues.data();
        
        vkCmdBeginRenderPass(cmd, &rpBegin, VK_SUBPASS_CONTENTS_INLINE);
        
        // 3. Ejecutar callbacks del pass
        if (compiled.definition.executeCallback) {
            compiled.definition.executeCallback(cmd, registry);
        }
        
        // 4. End render pass
        vkCmdEndRenderPass(cmd);
    }
}

void RenderGraph::cleanup(VkDevice device) {
    for (auto& compiled : compiledPasses) {
        if (compiled.framebuffer != VK_NULL_HANDLE) {
            vkDestroyFramebuffer(device, compiled.framebuffer, nullptr);
            compiled.framebuffer = VK_NULL_HANDLE;
        }
        if (compiled.vkRenderPass != VK_NULL_HANDLE) {
            vkDestroyRenderPass(device, compiled.vkRenderPass, nullptr);
            compiled.vkRenderPass = VK_NULL_HANDLE;
        }
    }
    
    compiledPasses.clear();
    compiledRegistry.resources.clear();
    compiledRegistry.nameToIndex.clear();
}

} // namespace FarmEngine::Render
