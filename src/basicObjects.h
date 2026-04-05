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
    glm::vec3 rotation=glm::vec3(0.0f); // radians (phi, theta)
    glm::vec3 scale=glm::vec3(1.0f);

    std::vector<VertexN> vertices;
    std::vector<uint32_t> indices;

    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;
    VkBuffer indexBuffer;
    VkDeviceMemory indexBufferMemory;

    BaseObject(
        glm::vec3 position=glm::vec3(0.0f), 
        glm::vec3 rotation=glm::vec3(0.0f), 
        glm::vec3 scale=glm::vec3(1.0f)){
            position = position;
            rotation = rotation;
            scale = scale;
    }
    void init(){
        createTriangles();
        applyTransformations();
    };

    

    virtual void createTriangles() = 0;

    void applyTransformations(){
        for (auto& vertex : vertices) {
            vertex.pos = getModelMatrix() * glm::vec4(vertex.pos, 1.0f);
            vertex.normal = glm::normalize(getModelMatrix() * glm::vec4(vertex.normal, 0.0f));
        }
    }

    glm::mat4 getModelMatrix(){
        glm::mat4 model = glm::mat4(1.0f);
        // 先平移
        model = glm::translate(model, position);
        
        model = glm::rotate(model, rotation.x, glm::vec3(1.0f, 0.0f, 1.0f)); // x轴旋转
        model = glm::rotate(model, rotation.y, glm::vec3(0.0f, 1.0f, 0.0f)); // y轴旋转
        model = glm::rotate(model, rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
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
        // 白色颜色常量
        glm::vec3 white(1.0f, 1.0f, 1.0f);
        
        // 定义6个面的顶点（每个面4个顶点，法线指向内部）
        // 面顺序：前、后、左、右、上、下
        // 坐标范围：-1 到 1
        
        // 前表面 (z = 1, 法线指向 -z)
        vertices.push_back({{-1, -1,  1}, { 0,  0, -1}, white});
        vertices.push_back({{ 1, -1,  1}, { 0,  0, -1}, white});
        vertices.push_back({{ 1,  1,  1}, { 0,  0, -1}, white});
        vertices.push_back({{-1,  1,  1}, { 0,  0, -1}, white});
        
        // 后表面 (z = -1, 法线指向 +z)
        vertices.push_back({{ 1, -1, -1}, { 0,  0,  1}, white});
        vertices.push_back({{-1, -1, -1}, { 0,  0,  1}, white});
        vertices.push_back({{-1,  1, -1}, { 0,  0,  1}, white});
        vertices.push_back({{ 1,  1, -1}, { 0,  0,  1}, white});
        
        // 左表面 (x = -1, 法线指向 +x)
        vertices.push_back({{-1, -1, -1}, { 1,  0,  0}, white});
        vertices.push_back({{-1, -1,  1}, { 1,  0,  0}, white});
        vertices.push_back({{-1,  1,  1}, { 1,  0,  0}, white});
        vertices.push_back({{-1,  1, -1}, { 1,  0,  0}, white});
        
        // 右表面 (x = 1, 法线指向 -x)
        vertices.push_back({{ 1, -1,  1}, {-1,  0,  0}, white});
        vertices.push_back({{ 1, -1, -1}, {-1,  0,  0}, white});
        vertices.push_back({{ 1,  1, -1}, {-1,  0,  0}, white});
        vertices.push_back({{ 1,  1,  1}, {-1,  0,  0}, white});
        
        // 上表面 (y = 1, 法线指向 -y)
        vertices.push_back({{-1,  1,  1}, { 0, -1,  0}, white});
        vertices.push_back({{ 1,  1,  1}, { 0, -1,  0}, white});
        vertices.push_back({{ 1,  1, -1}, { 0, -1,  0}, white});
        vertices.push_back({{-1,  1, -1}, { 0, -1,  0}, white});
        
        // 下表面 (y = -1, 法线指向 +y)
        vertices.push_back({{-1, -1, -1}, { 0,  1,  0}, white});
        vertices.push_back({{ 1, -1, -1}, { 0,  1,  0}, white});
        vertices.push_back({{ 1, -1,  1}, { 0,  1,  0}, white});
        vertices.push_back({{-1, -1,  1}, { 0,  1,  0}, white});
        
        // 生成索引（每个面两个三角形）
        for (uint32_t face = 0; face < 6; face++) {
            uint32_t baseIndex = face * 4;
            // 三角形1: 顶点0,1,2
            indices.push_back(baseIndex);
            indices.push_back(baseIndex + 1);
            indices.push_back(baseIndex + 2);
            // 三角形2: 顶点0,2,3
            indices.push_back(baseIndex);
            indices.push_back(baseIndex + 2);
            indices.push_back(baseIndex + 3);
        }
    }
};

class Cube : public BaseObject{
public:
    void createTriangles() override{
        // 白色颜色常量
        glm::vec3 white(1.0f, 1.0f, 1.0f);
        
        // 定义6个面的顶点（每个面4个顶点，法线指向内部）
        // 面顺序：前、后、左、右、上、下
        // 坐标范围：-1 到 1
        
        // 前表面 (z = 1, 法线指向 z)
        vertices.push_back({{-1, -1,  1}, { 0,  0, 1}, white});
        vertices.push_back({{ 1, -1,  1}, { 0,  0, 1}, white});
        vertices.push_back({{ 1,  1,  1}, { 0,  0, 1}, white});
        vertices.push_back({{-1,  1,  1}, { 0,  0, 1}, white});
        
        // 后表面 (z = -1, 法线指向 -z)
        vertices.push_back({{ 1, -1, -1}, { 0,  0, -1}, white});
        vertices.push_back({{-1, -1, -1}, { 0,  0, -1}, white});
        vertices.push_back({{-1,  1, -1}, { 0,  0, -1}, white});
        vertices.push_back({{ 1,  1, -1}, { 0,  0, -1}, white});
        
        // 左表面 (x = -1, 法线指向 -x)
        vertices.push_back({{-1, -1, -1}, { -1,  0,  0}, white});
        vertices.push_back({{-1, -1,  1}, {-1,  0,  0}, white});
        vertices.push_back({{-1,  1,  1}, { -1,  0,  0}, white});
        vertices.push_back({{-1,  1, -1}, { -1,  0,  0}, white});
        
        // 右表面 (x = 1, 法线指向 x)
        vertices.push_back({{ 1, -1,  1}, {1,  0,  0}, white});
        vertices.push_back({{ 1, -1, -1}, {1,  0,  0}, white});
        vertices.push_back({{ 1,  1, -1}, {1,  0,  0}, white});
        vertices.push_back({{ 1,  1,  1}, {1,  0,  0}, white});
        
        // 上表面 (y = 1, 法线指向 y)
        vertices.push_back({{-1,  1,  1}, { 0, 1,  0}, white});
        vertices.push_back({{ 1,  1,  1}, { 0, 1,  0}, white});
        vertices.push_back({{ 1,  1, -1}, { 0, 1,  0}, white});
        vertices.push_back({{-1,  1, -1}, { 0, 1,  0}, white});
        
        // 下表面 (y = -1, 法线指向 -y)
        vertices.push_back({{-1, -1, -1}, { 0, -1,  0}, white});
        vertices.push_back({{ 1, -1, -1}, { 0, -1,  0}, white});
        vertices.push_back({{ 1, -1,  1}, { 0, -1,  0}, white});
        vertices.push_back({{-1, -1,  1}, { 0, -1,  0}, white});
        
        // 生成索引（每个面两个三角形）
        for (uint32_t face = 0; face < 6; face++) {
            uint32_t baseIndex = face * 4;
            // 三角形1: 顶点0,1,2
            indices.push_back(baseIndex);
            indices.push_back(baseIndex + 1);
            indices.push_back(baseIndex + 2);
            // 三角形2: 顶点0,2,3
            indices.push_back(baseIndex);
            indices.push_back(baseIndex + 2);
            indices.push_back(baseIndex + 3);
        }
    }
};

