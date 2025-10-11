#pragma once
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#include <string>

class BaseRenderer {
public:
    std::string description = "Base Vulkan Renderer";

    void initVulkan();
    void drawFrame();
    void cleanupVulkan();

private:
    VkBuffer vkBuffer; // Example usage of VkBuffer from vulkan_core.h
};