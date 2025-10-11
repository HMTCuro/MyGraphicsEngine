#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE 
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>

#include <iostream>
#include <string>
#include <vector>

#include <renderer.h>

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
    BaseRenderer* renderer;

    void initWindow() {
        glfwInit();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

        window = glfwCreateWindow(WIDTH,HEIGHT,"Renderer Window", nullptr, nullptr);
        glfwSetWindowUserPointer(window, this);
        
    }

    void initVulkan(){
        renderer->initVulkan();
    }

    void mainLoop(){
        while(glfwWindowShouldClose(window) == 0){
            glfwPollEvents();
        }
    }

    void cleanup(){
        glfwDestroyWindow(window);
        glfwTerminate();
        renderer->cleanupVulkan();
    }
};



int main() {
    Application app;
    app.run();

    return 0;
}
