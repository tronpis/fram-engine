#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <string>
#include <optional>
#include <memory>

namespace FarmEngine {

struct VulkanDeviceConfig {
    bool enableValidationLayers = true;
    bool enableDebugUtils = true;
    uint32_t desiredImageCount = 3;
};

class VulkanInstance {
public:
    VulkanInstance();
    ~VulkanInstance();
    
    bool initialize(const VulkanDeviceConfig& config);
    VkInstance getInstance() const { return instance; }
    
private:
    bool createInstance(const VulkanDeviceConfig& config);
    bool setupDebugMessenger();
    void populateValidationLayers();
    void populateRequiredExtensions();
    
    VkInstance instance = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;
    std::vector<const char*> validationLayers;
    std::vector<const char*> requiredExtensions;
};

struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;
    
    bool isComplete() const {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

class VulkanPhysicalDevice {
public:
    VulkanPhysicalDevice(VkInstance instance, VkSurfaceKHR surface);
    
    bool isSuitable();
    void select();
    
    VkPhysicalDevice getDevice() const { return physicalDevice; }
    QueueFamilyIndices getQueueFamilies() const { return queueFamilyIndices; }
    
private:
    bool checkExtensionSupport();
    bool checkValidationLayerSupport();
    void queryQueueFamilies();
    
    VkInstance instance;
    VkSurfaceKHR surface;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    QueueFamilyIndices queueFamilyIndices;
};

class VulkanLogicalDevice {
public:
    VulkanLogicalDevice(VkPhysicalDevice physicalDevice, 
                       VkSurfaceKHR surface,
                       const QueueFamilyIndices& indices);
    ~VulkanLogicalDevice();
    
    VkDevice getDevice() const { return device; }
    VkQueue getGraphicsQueue() const { return graphicsQueue; }
    VkQueue getPresentQueue() const { return presentQueue; }
    
private:
    void createDevice(const QueueFamilyIndices& indices);
    void getQueues(const QueueFamilyIndices& indices);
    
    VkDevice device = VK_NULL_HANDLE;
    VkQueue graphicsQueue = VK_NULL_HANDLE;
    VkQueue presentQueue = VK_NULL_HANDLE;
};

class VulkanSwapchain {
public:
    VulkanSwapchain(VkDevice device, 
                   VkPhysicalDevice physicalDevice,
                   VkSurfaceKHR surface,
                   VkQueue presentQueue,
                   uint32_t imageCount);
    ~VulkanSwapchain();
    
    VkSwapchainKHR getSwapchain() const { return swapchain; }
    const std::vector<VkImage>& getImages() const { return images; }
    VkFormat getImageFormat() const { return imageFormat; }
    VkExtent2D getExtent() const { return extent; }
    
private:
    VkSurfaceFormatKHR chooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats);
    VkPresentModeKHR choosePresentMode(const std::vector<VkPresentModeKHR>& modes);
    VkExtent2D chooseExtent(const VkSurfaceCapabilitiesKHR& capabilities);
    
    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    std::vector<VkImage> images;
    VkFormat imageFormat;
    VkExtent2D extent{};
};

class VulkanContext {
public:
    static VulkanContext& getInstance();
    
    bool initialize(void* windowHandle, void* windowInstance, const VulkanDeviceConfig& config = {});
    void cleanup();
    
    // Getters
    VkInstance getInstance() const { return vulkanInstance.getInstance(); }
    VkPhysicalDevice getPhysicalDevice() const { return physicalDevice.getDevice(); }
    VkDevice getDevice() const { return logicalDevice->getDevice(); }
    VkSurfaceKHR getSurface() const { return surface; }
    VkQueue getGraphicsQueue() const { return logicalDevice->getGraphicsQueue(); }
    VkQueue getPresentQueue() const { return logicalDevice->getPresentQueue(); }
    VkSwapchainKHR getSwapchain() const { return swapchain->getSwapchain(); }
    
    const std::vector<VkImage>& getSwapchainImages() const { return swapchain->getImages(); }
    VkFormat getSwapchainImageFormat() const { return swapchain->getImageFormat(); }
    VkExtent2D getSwapchainExtent() const { return swapchain->getExtent(); }
    
private:
    VulkanContext() = default;
    ~VulkanContext();
    
    VulkanContext(const VulkanContext&) = delete;
    VulkanContext& operator=(const VulkanContext&) = delete;
    
    bool createSurface(void* windowHandle, void* windowInstance);
    
    VulkanInstance vulkanInstance;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VulkanPhysicalDevice physicalDevice{nullptr, nullptr};
    std::unique_ptr<VulkanLogicalDevice> logicalDevice;
    std::unique_ptr<VulkanSwapchain> swapchain;
};

} // namespace FarmEngine
