#pragma once

#include "renderer.h"

const std::vector<const char*> rayTracingDeviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
    VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
    VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
    VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
    VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
    VK_KHR_SPIRV_1_4_EXTENSION_NAME,
    VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME
};

// Holds data for a scratch buffer used as a temporary storage during acceleration structure builds
struct ScratchBuffer
{
	uint64_t       deviceAddress;
	VkBuffer       handle;
	VkDeviceMemory memory;
};

// Wraps all data required for an acceleration structure
struct AccelerationStructure
{
	VkAccelerationStructureKHR          handle;
	uint64_t                            deviceAddress;
	VkBuffer                            buffer;
    VkDeviceMemory                      memory;
};

class BaseRayTracingRenderer{
public:
    GLFWwindow* window; // Pointer to the GLFW window

    void initVulkan();
    void drawFrame();
    void cleanupVulkan();

    void waitIdle();
    void setFramebufferResizeCallback();

private:
    // Example variables
    std::vector<BaseObjects> objects;
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

    std::vector<Vertex> verticesTLAS;
    std::vector<uint32_t> indicesTLAS;
    VkBuffer vertexBufferTLAS;
    VkDeviceMemory vertexBufferTLASMemory;
    VkBuffer indexBufferTLAS;
    VkDeviceMemory indexBufferTLASMemory;

    std::vector<VkBuffer> uniformBuffers;
    std::vector<VkDeviceMemory> uniformBuffersMemory;
    std::vector<void*> uniformBuffersMapped;
    size_t uboSize = 0;
    uint32_t currentFrame = 0;
    bool framebufferResized = false;
    bool usingComputePipeline = false;

    // ray tracing specific variables
    VkPhysicalDeviceAccelerationStructureFeaturesKHR accelerationStructureFeatures{};
    VkPhysicalDeviceRayTracingPipelineFeaturesKHR rayTracingPipelineFeatures{};
    VkPhysicalDeviceBufferDeviceAddressFeatures bufferDeviceAddressFeatures{};

    VkPipeline rayTracingPipeline;
    VkPipelineLayout rayTracingPipelineLayout;
    
    AccelerationStructure blas;
    AccelerationStructure tlas;


    VkBuffer rayTracingShaderBindingTableBuffer;
    VkDeviceMemory rayTracingShaderBindingTableBufferMemory;
    

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
    void createAccelerationStructures(); // Necessary: Create acceleration structures for ray tracing
    void createBLAS();
    void createTLAS();

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
        auto renderer = reinterpret_cast<BaseRayTracingRenderer*>(glfwGetWindowUserPointer(window));
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
    void loadObjects();
    VkDeviceAddress getBufferDeviceAddress(VkBuffer buffer);

};