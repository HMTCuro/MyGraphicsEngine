#include <iostream>
#include <vector>
#include <string>
#include <set>

#include "renderer.h"

#include "../shaders/vsh.cpp"
#include "../shaders/fsh.cpp"

void BaseRenderer::initVulkan() {
    // Initialize Vulkan instance, devices, swapchain, etc.
    createInstance();
    setupDebugMessenger();
    if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
        throw std::runtime_error("failed to create window surface!");
    }
    pickPhysicalDevice();
    createLogicalDevice();
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
    // std::cout << "Frame Drawn" << std::endl;
}

void BaseRenderer::cleanupVulkan() {
    // Cleanup Vulkan resources
    std::cout << "Vulkan Cleaned Up" << std::endl;

    vkDestroyDevice(device, nullptr);
    vkDestroySurfaceKHR(instance, surface, nullptr);
    // vkDestroyDebugUtilsMessengerEXT
    if(enableValidationLayers){
        auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
        if(func != nullptr){
            func(instance, debugMessenger, nullptr);
        }
    }
    vkDestroyInstance(instance, nullptr);
}

void BaseRenderer::createInstance(){
    std::cout << "Creating Vulkan Instance" << std::endl;

    // Get required extensions from GLFW
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    std::vector<const char*> extensions (glfwExtensions, glfwExtensions + glfwExtensionCount);

    for (auto &ext: extensions){
        std::cout << "Required GLFW Extension: " << ext << std::endl;
    }

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
    if(enableValidationLayers){
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
    }
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);
    if(result != VK_SUCCESS){
        throw std::runtime_error("failed to create instance!");
    }
}

void BaseRenderer::setupDebugMessenger(){
    if(!enableValidationLayers) return;

    VkDebugUtilsMessengerCreateInfoEXT createInfo{};
    createInfo.sType= VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugCallback;
    createInfo.pUserData = nullptr;

    // Explaination: vkCreateDebugUtilsMessengerEXT is an extension function and not automatically loaded.
    // We need to look up its address using vkGetInstanceProcAddr and then cast it to the appropriate function pointer type.
    // This is a common pattern in Vulkan for using extension functions.
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        VkResult result = func(instance, &createInfo, nullptr, &debugMessenger);
        if (result != VK_SUCCESS) {
            throw std::runtime_error("failed to set up debug messenger!");
        }
    } else {
        throw std::runtime_error("failed to find vkCreateDebugUtilsMessengerEXT function!");
    }
}

void BaseRenderer::pickPhysicalDevice(){
    uint32_t deviceCount=0;
    vkEnumeratePhysicalDevices(instance,&deviceCount,nullptr);
    if(deviceCount == 0){
        throw std::runtime_error("failed to find GPUs with Vulkan support!");
    }
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance,&deviceCount,devices.data());

    for(const auto& device: devices){
        bool showDeviceInfo = true;
        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(device, &deviceProperties);
        VkPhysicalDeviceFeatures deviceFeatures;
        vkGetPhysicalDeviceFeatures(device, &deviceFeatures);
        if(showDeviceInfo) {
            std::cout << "Device: " << device << std::endl;
            std::cout << "Device Name: " << deviceProperties.deviceName << std::endl;
            std::cout << "Device Type: " << deviceProperties.deviceType << std::endl;
            std::cout << "API Version: " << deviceProperties.apiVersion << std::endl;
            std::cout << "Geometry Shader Support: " << (deviceFeatures.geometryShader ? "Yes" : "No") << std::endl;
        }
        std::string deviceName = deviceProperties.deviceName;
        if (deviceName.find("NVIDIA") != std::string::npos) {
            physicalDevice = device;
            break; // Prefer NVIDIA GPU if available
        }

        // TODO: Check for required features and extensions
    }

    if (physicalDevice == VK_NULL_HANDLE) {
        physicalDevice = devices[0]; // Fallback to the first device if no NVIDIA GPU found
    }

    if(physicalDevice == VK_NULL_HANDLE){
        throw std::runtime_error("failed to find a suitable GPU!");
    }

}

void BaseRenderer::createLogicalDevice(){
    QueueFamilyIndices indices= findQueueFamilies(physicalDevice);
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<int32_t> uniqueQueueFamilies = {indices.graphicsFamily, indices.presentFamily};

    // priority: 0.0 is the lowest, 1.0 is the highest
    float queuePriority = 1.0f;

    for(int32_t queueFamily:uniqueQueueFamilies){
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = static_cast<uint32_t>(queueFamily);
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfo.pNext = nullptr;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    VkPhysicalDeviceFeatures deviceFeatures{};

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount =static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos=queueCreateInfos.data();
    createInfo.pEnabledFeatures = &deviceFeatures;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();
    if(enableValidationLayers){
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
    } else {
        createInfo.enabledLayerCount = 0;
    }


    VkResult result = vkCreateDevice(physicalDevice, &createInfo, nullptr, &device);
    if(result != VK_SUCCESS){
        throw std::runtime_error("failed to create logical device!");
    }
}


QueueFamilyIndices BaseRenderer::findQueueFamilies(VkPhysicalDevice device){
    QueueFamilyIndices indices;
    uint32_t queueFamilyCount=0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());
    int i = 0;
    for(const auto& queueFamily:queueFamilies){
        if(queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.graphicsFamily = i;
        }
        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
        if (presentSupport) {
            indices.presentFamily = i;
        }
        if (indices.isComplete()) {
            break;
        }
        i++;
    }
    return indices;
} 
