// VkExtensions (Before including Vulkan headers)
// for ray tracing
#define VK_KHR_acceleration_structure 1
#define VK_KHR_ray_tracing_pipeline 1
#define VK_KHR_buffer_device_address 1
#define VK_KHR_spirv_1_4 1

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <string>
#include <vector>

#include <renderer.h>
#include "RayTracingRenderer.h"

const uint32_t WIDTH=1920;
const uint32_t HEIGHT=1080;

class Application {
public:
    void run() {
        initWindow();
        initVulkan();
        mainLoop();
        cleanup();
    }
private:
    //Variables
    GLFWwindow* window;
    // BaseRenderer renderer;
    BaseRayTracingRenderer renderer;

    void initWindow() {
        glfwInit();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

        window = glfwCreateWindow(WIDTH,HEIGHT,"Renderer Window", nullptr, nullptr);
        glfwSetWindowUserPointer(window, this);
        renderer.window = window;
        renderer.setFramebufferResizeCallback();
    }

    void initVulkan(){
        renderer.initVulkan();
    }

    void mainLoop(){
        while(glfwWindowShouldClose(window) == 0){
            glfwPollEvents();
            renderer.drawFrame();
            // break;
        }
        renderer.waitIdle(); 
    }

    void cleanup(){
        renderer.cleanupVulkan();
        glfwDestroyWindow(window);
        glfwTerminate();
    }
};



int main() {
    Application app;

    app.run();

    return 0;
}
