#include <iostream>
#include "renderer.h"

#include "../shaders/vsh.cpp"
#include "../shaders/fsh.cpp"

void BaseRenderer::initVulkan() {
    // Initialize Vulkan instance, devices, swapchain, etc.
    createInstance();
    //     setupDebugMessenger();
    //     createSurface();
    //     pickPhysicalDevice();
    //     createLogicalDevice();
    //     createSwapChain();
    //     createImageViews();
    //     createRenderPass();
    //     createDescriptorSetLayout();
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
    std::cout << "Vulkan Initialized" << std::endl;
}

void BaseRenderer::drawFrame() {
    // Record commands and submit to the GPU
    std::cout << "Frame Drawn" << std::endl;
}

void BaseRenderer::cleanupVulkan() {
    // Cleanup Vulkan resources
    std::cout << "Vulkan Cleaned Up" << std::endl;
}

void BaseRenderer::createInstance(){
    std::cout << "Creating Vulkan Instance" << std::endl;

    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "VkEngine";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType= VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = 0;

}