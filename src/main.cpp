#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define GLM_FORCE_DEPTH_ZERO_TO_ONE // In OpenGL, the depth range is -1 to 1, but in Vulkan it is 0 to 1.
#define GLM_ENABLE_EXPERIMENTAL

#include <iostream>
#include <string>
#include <vector>

#include <renderer.h>

int main() {
    BaseRenderer* renderer = new BaseRenderer();
    std::cout << renderer->description << std::endl;

    delete renderer;
    return 0;
}