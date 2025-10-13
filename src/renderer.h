#pragma once
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE 
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>

#include <string>
#include <vector>

struct Vertex {
    glm::vec3 pos;
    glm::vec3 color;
    // glm::vec2 texCoord;

};

class BaseRenderer {
public:
    std::string description = "Base Vulkan Renderer";

    void initVulkan();
    void drawFrame();
    void cleanupVulkan();

private:
    // Example variables
    std::vector<Vertex> vertices;
    VkBuffer vkBuffer; // Example usage of VkBuffer from vulkan_core.h

    // Core variables
    VkInstance instance;

    void createInstance();
    void createVertexBuffer();
    void destroyVertexBuffer();
};