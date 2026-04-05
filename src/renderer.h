#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE 
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

// #include <glslang/Public/ShaderLang.h>

#include <string>
#include <vector>
#include <iostream>
#include <set>
#include <cstring>
#include <fstream>
#include <chrono>

#include "basicObjects.h"

#ifndef NDEBUG
    const bool enableValidationLayers = true;
#else
    const bool enableValidationLayers = false;
#endif

const uint32_t MAX_FRAMES_IN_FLIGHT = 2;

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


struct MinimumUBO{
    float time;

    void update(){
        static auto startTime = std::chrono::high_resolution_clock::now();
        auto currentTime = std::chrono::high_resolution_clock::now();
        float deltaTime = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

        time = deltaTime;
    }
};

class BaseRenderer {
public:
    std::string description = "Base Vulkan Renderer";

    GLFWwindow* window; // Pointer to the GLFW window

    void initVulkan();
    void drawFrame();
    void cleanupVulkan();

    void waitIdle();
    void setFramebufferResizeCallback();
private:
    // Example variables
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
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
    VkPipelineLayout pipelineLayout;
    VkPipeline graphicsPipeline; // Basic rasterization pipeline
    VkCommandPool commandPool;
    std::vector<VkCommandBuffer> commandBuffers;
    VkDescriptorPool descriptorPool;
    std::vector<VkDescriptorSet> descriptorSets;
    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;
    VkPipeline computePipeline;
    VkPipelineLayout computePipelineLayout;
    VkDescriptorSetLayout computeDescriptorSetLayout;

    VkImage colorImage;
    VkDeviceMemory colorImageMemory;
    VkImageView colorImageView;
    VkImage depthImage;
    VkDeviceMemory depthImageMemory;
    VkImageView depthImageView;

    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;
    VkBuffer indexBuffer;
    VkDeviceMemory indexBufferMemory;

    std::vector<VkBuffer> uniformBuffers;
    std::vector<VkDeviceMemory> uniformBuffersMemory;
    std::vector<void*> uniformBuffersMapped;
    size_t uboSize = 0;
    uint32_t currentFrame = 0;
    bool framebufferResized = false;
    bool usingComputePipeline = false;

    // Core functions
    void createInstance(); // Necessary: Create Vulkan instance
    void setupDebugMessenger(); // Optional: Setup debug messenger for validation layers
    void pickPhysicalDevice(); // Necessary: Select physical device (GPU)
    void createLogicalDevice(); // Necessary: Create logical device from physical device
    void createSwapChain(); // Necessary: Create swap chain for rendering
    void recreateSwapChain(); // Necessary: Recreate swap chain on window resize
    void createImageViews(); // Necessary: Create image views for swap chain images
    void createRenderPass(); // Necessary: Create render pass for framebuffers
    void createDescriptorSetLayout(); // Necessary: Create descriptor set layout
    void createGraphicsPipeline(); // Necessary: Create graphics pipeline
    void createCommandPool(); // Necessary: Create command pool for command buffers
    void createColorResources(); // Necessary: Create color resources (image, image view, and sampler)
    void createDepthResources(); // Necessary: Create depth resources (image, image view, and sampler)
    void createFramebuffers(); // Necessary: Create framebuffers for the swap chain
    //     createTextureImage();
    //     createTextureImageView();
    //     createTextureSampler();
    //     loadModel();
    void createVertexBuffer(); // Necessary: Create vertex buffer
    void createIndexBuffer(); // Necessary: Create index buffer
    template<typename T>
    void createUniformBuffers();
    template<typename T>
    void updateUniformBuffer(uint32_t currentImage);
    void createDescriptorPool();
    void createDescriptorSets();
    void createCommandBuffers(); // Necessary: Create command buffers
    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
    void createSyncObjects(); // Necessary: Create synchronization objects
    void cleanupSwapChain(); // Necessary: Cleanup swap chain resources

    void createComputePipeline(); // Editable: Create compute pipeline
    // Supplementary functions
    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) {

        std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

        return VK_FALSE;
    }

    static void framebufferResizedCallback(GLFWwindow* window, int width, int height) {
        auto renderer = reinterpret_cast<BaseRenderer*>(glfwGetWindowUserPointer(window));
        renderer->framebufferResized = true;
    }
    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);
    VkImageView createImageView(VkImage image, VkFormat format,VkImageAspectFlags aspectFlags, uint32_t miplevels=1); // Editable: Create a single image view
    VkFormat findDepthFormat();
    VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
    std::vector<char> readSpvFile(const std::string& filename);
    VkShaderModule createShaderModule(const std::vector<char>& code);
    void createImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
    VkCommandBuffer beginSingleTimeCommands();
    void endSingleTimeCommands(VkCommandBuffer commandBuffer);
    void loadVertexAndIndex();
};

extern template void BaseRenderer::createUniformBuffers<MinimumUBO>();
extern template void BaseRenderer::updateUniformBuffer<MinimumUBO>(uint32_t currentImage);

template<typename T>
struct UBOConfig{
    T ubo;
    VkDevice device;
    VkBuffer uniformBuffer;
    VkDeviceMemory uniformBufferMemory;
};
template struct UBOConfig<MinimumUBO>;

template<typename T>
class UboManager {
public:
    UboManager(UBOConfig<T> config) {
        this->config = config;
    }

    void createUniformBuffer();
    void updateUniformBuffer(uint32_t currentImage);
private:
    UBOConfig<T> config;

};
extern template class UboManager<MinimumUBO>;