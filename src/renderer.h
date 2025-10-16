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

    // Core functions
    void createInstance(); // Necessary: Create Vulkan instance
    void setupDebugMessenger(); 
    void pickPhysicalDevice(); // Necessary: Select physical device (GPU)
    void createLogicalDevice();
    void createVertexBuffer();
    void destroyVertexBuffer();

    // Supplementary functions
    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) {

        std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

        return VK_FALSE;
    }

    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
};