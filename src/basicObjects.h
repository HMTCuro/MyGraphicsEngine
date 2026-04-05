#pragma once

#include <vulkan/vulkan.h>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE 
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <vector>

struct Vertex {
    glm::vec3 pos;
    glm::vec3 color;
    // glm::vec2 texCoord;

    static VkVertexInputBindingDescription getBindingDescription() {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Vertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return bindingDescription;
    }

    static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions() {
        std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};

        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(Vertex, pos);

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(Vertex, color);

        return attributeDescriptions;
    }

};

struct VertexN {
    glm::vec3 pos;
    glm::vec3 normal;
    glm::vec3 color;
    // glm::vec2 texCoord;

    static VkVertexInputBindingDescription getBindingDescription() {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(VertexN);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return bindingDescription;
    }

    static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions() {
        std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};

        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(VertexN, pos);

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(VertexN, normal);

        attributeDescriptions[2].binding = 0;
        attributeDescriptions[2].location = 2;
        attributeDescriptions[2].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[2].offset = offsetof(VertexN, color);

        return attributeDescriptions;
    }

};

class BaseObject{
public:
    glm::vec3 position=glm::vec3(0.0f);
    glm::vec2 rotation=glm::vec2(0.0f); // radians (phi, theta)
    glm::vec3 scale=glm::vec3(1.0f);

    std::vector<VertexN> vertices;
    std::vector<uint32_t> indices;

    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;
    VkBuffer indexBuffer;
    VkDeviceMemory indexBufferMemory;

    void init(){
        createTriangles();
    };

    

    virtual void createTriangles() = 0;

    glm::mat4 getModelMatrix(){
        glm::mat4 model = glm::mat4(1.0f);
        // 先平移
        model = glm::translate(model, position);
        // 先绕z轴旋转（xy平面角），再绕x轴旋转（仰角）
        model = glm::rotate(model, rotation.x, glm::vec3(0.0f, 0.0f, 1.0f)); // xy平面角
        model = glm::rotate(model, rotation.y, glm::vec3(1.0f, 0.0f, 0.0f)); // 仰角
        // 缩放
        model = glm::scale(model, scale);
        return model;
    }

private:

};

class Plane : public BaseObject{
public:
    void createTriangles() override{
        vertices = {
            {{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f, 1.0f}},
            {{1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f, 1.0f}},
            {{1.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f, 1.0f}},
            {{0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f, 1.0f}}
        };
        indices = {
            0, 1, 2,
            2, 3, 0
        };
    }
};

class DebuggerTriangle : public BaseObject{
public:
    void createTriangles() override{
        vertices = {
            {{0.0f, -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f, 1.0f}},
            {{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f, 1.0f}},
            {{-0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f, 1.0f}}
        };

        indices = {
            0, 1, 2
        };

    }
};

// class Cube : public BaseObject{
// public:
//     void createTriangles() override{
//         float radius = 0.5f;
//         float 
//     }
// };

class Room : public BaseObject{
public:
    void createTriangles() override{
        vertices = {
            {{-1.0f, -1.0f, -1.0f}, glm::normalize(glm::vec3(-1.0f, -1.0f, -1.0f)), {1.0f, 1.0f, 1.0f}},
            {{-1.0f, -1.0f,  1.0f}, glm::normalize(glm::vec3(-1.0f, -1.0f, 1.0f)), {1.0f, 1.0f, 1.0f}},
            {{ 1.0f, -1.0f, -1.0f}, glm::normalize(glm::vec3(1.0f, -1.0f, -1.0f)), {1.0f, 1.0f, 1.0f}},
            {{ 1.0f, -1.0f,  1.0f}, glm::normalize(glm::vec3(1.0f, -1.0f, 1.0f)), {1.0f, 1.0f, 1.0f}},
            {{-1.0f,  1.0f, -1.0f}, glm::normalize(glm::vec3(-1.0f, 1.0f, -1.0f)), {1.0f, 1.0f, 1.0f}},
            {{-1.0f,  1.0f,  1.0f}, glm::normalize(glm::vec3(-1.0f, 1.0f, 1.0f)), {1.0f, 1.0f, 1.0f}},
            {{ 1.0f,  1.0f, -1.0f}, glm::normalize(glm::vec3(1.0f, 1.0f, -1.0f)), {1.0f, 1.0f, 1.0f}},
            {{ 1.0f,  1.0f,  1.0f}, glm::normalize(glm::vec3(1.0f, 1.0f, 1.0f)), {1.0f, 1.0f, 1.0f}},
        }; // 8 vertices, normals = norm(pos), color =white
        //     
        indices = {
            0, 3, 1, 0, 2, 3, // floor
            4, 5, 6, 5, 7, 6, // ceiling
            0, 1, 4, 1, 5, 4, // wall 1
            2, 6, 3, 6, 7, 3, // wall 2
            0, 6, 2, 6, 0, 4, // wall 3
            1, 3, 7, 7, 5, 1 // wall 4
        }; 
    }
};

