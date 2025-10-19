#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE 
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>

#include <string>
#include <vector>

#ifndef NDEBUG
    const bool enableValidationLayers = true;
#else
    const bool enableValidationLayers = false;
#endif

const std::vector<const char*> validationLayers={
    "VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

struct QueueFamilyIndices{
    int32_t graphicsFamily= -1;
    int32_t presentFamily= -1;

    bool isComplete(){
        return graphicsFamily != -1 && presentFamily != -1;
    }
};

struct SwapChainSupportDetails{
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

struct Vertex {
    glm::vec3 pos;
    glm::vec3 color;
    // glm::vec2 texCoord;

};

class BaseRenderer {
public:
    std::string description = "Base Vulkan Renderer";

    GLFWwindow* window; // Pointer to the GLFW window

    void initVulkan();
    void drawFrame();
    void cleanupVulkan();

private:
    // Example variables
    std::vector<Vertex> vertices;
    VkBuffer vkBuffer; // Example usage of VkBuffer from vulkan_core.h

    // Core variables
    VkInstance instance;
    VkDebugUtilsMessengerEXT debugMessenger;
    VkSurfaceKHR surface;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device;
    VkQueue graphicsQueue;
    VkQueue presentQueue;
    VkSwapchainKHR swapChain;
    std::vector<VkImage> swapChainImages;
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;
    std::vector<VkImageView> swapChainImageViews;
    std::vector<VkFramebuffer> swapChainFramebuffers;
    VkRenderPass renderPass;
    VkDescriptorSetLayout descriptorSetLayout;

    // Core functions
    void createInstance(); // Necessary: Create Vulkan instance
    void setupDebugMessenger(); // Optional: Setup debug messenger for validation layers
    void pickPhysicalDevice(); // Necessary: Select physical device (GPU)
    void createLogicalDevice(); // Necessary: Create logical device from physical device
    void createSwapChain(); // Necessary: Create swap chain for rendering
    void createImageViews(); // Necessary: Create image views for swap chain images
    void createRenderPass(); // Necessary: Create render pass for framebuffers
    void createDescriptorSetLayout();
    //     createGraphicsPipeline();
    //     createCommandPool();
    //     createColorResources();
    //     createDepthResources();
    //     createFramebuffers();
    //     createTextureImage();
    //     createTextureImageView();
    //     createTextureSampler();
    //     loadModel();
    //     createVertexBuffer();
    //     createIndexBuffer();
    //     createUniformBuffers();
    //     // MyLight
    //     // createLightUniformBuffers();
    //     createDescriptorPool();
    //     createDescriptorSets();
    //     createCommandBuffers();
    //     createSyncObjects();
    void createVertexBuffer(); // Example: Create vertex buffer
    void destroyVertexBuffer(); // Example: Destroy vertex buffer
    void cleanupSwapChain(); // Necessary: Cleanup swap chain resources
    // Supplementary functions
    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) {

        std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

        return VK_FALSE;
    }

    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);
    VkImageView createImageView(VkImage image, VkFormat format,VkImageAspectFlags aspectFlags, uint32_t miplevels=1); // Editable: Create a single image view
    VkFormat findDepthFormat();
    VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
};