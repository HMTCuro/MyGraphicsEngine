#include <iostream>
#include "renderer.h"

#include "../shaders/vsh.cpp"
#include "../shaders/fsh.cpp"

void BaseRenderer::initVulkan() {
    // Initialize Vulkan instance, devices, swapchain, etc.
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