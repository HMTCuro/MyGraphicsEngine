#pragma once

#include <vulkan/vulkan.h>

// #define GLM_FORCE_DEPTH_ZERO_TO_ONE 
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>

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

struct PointLight{
    alignas(16) glm::vec3 pos;
    alignas(16) glm::vec3 color;
    alignas(4) float intensity;
};

struct Camera{
    alignas(16) glm::vec3 pos;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 project;
};

class UniformBuffer{
public:
    std::vector<VkBuffer> buffer;
    std::vector<VkDeviceMemory> bufferMemory;
    std::vector<void*> bufferMapped;
};

class StorageBuffer{
public:
    std::vector<VkBuffer> buffer;
    std::vector<VkDeviceMemory> bufferMemory;
    std::vector<void*> bufferMapped;
};

struct CameraParameters{
    glm::vec3 position=glm::vec3(0.0f);
    glm::vec3 direction=glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 pitchYawRoll=glm::vec3(0.0f); // radians
    float fov=glm::radians(45.0f);
    float aspectRatio=1.0f;
    float nearPlane=0.1f;   
    float farPlane=100.0f;

    Camera getCamera() const {
        Camera cam;
        cam.pos = position;
        glm::mat4 rotation = glm::yawPitchRoll(pitchYawRoll.y, pitchYawRoll.x, pitchYawRoll.z);
        cam.view = glm::lookAtLH(position, position + glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, 1.0f, 0.0f)) * rotation;
        cam.project = glm::perspectiveLH(fov, aspectRatio, nearPlane, farPlane);
        return cam;
    }
};

class CameraInstance{
public:
    Camera data;
    UniformBuffer uniformBuffer;
};

struct GlobalInfo{
    alignas(16) glm::vec2 windowSize;
    alignas(16) glm::vec3 backgroundColor;
    alignas(4) int scale;
};

class GlobalInfoInstance{
public:
    GlobalInfo data;
    UniformBuffer uniformBuffer;
};

struct Timer{
    float time;

    void update(){
        static auto startTime = std::chrono::high_resolution_clock::now();
        auto currentTime = std::chrono::high_resolution_clock::now();
        float deltaTime = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

        time = deltaTime;
    }
};

class TimerInstance{
public:
    Timer data;
    UniformBuffer uniformBuffer;
};

class PointLightInstance{
public:
    PointLight data;
    UniformBuffer uniformBuffer;
};

class Mesh{
public:
    uint32_t id;
    std::vector<VertexN> vertices;
    std::vector<uint32_t> indices;

    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;
    VkBuffer indexBuffer;
    VkDeviceMemory indexBufferMemory;

    Mesh() = default;
};

struct MeshInstance{
    Mesh* mesh;
    glm::vec3 position=glm::vec3(0.0f);
    glm::vec3 rotation=glm::vec3(0.0f); // radians
    glm::vec3 scale=glm::vec3(1.0f);

    glm::mat4 getModelMatrix() const {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, position);
        
        model = glm::rotate(model, rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
        model = glm::rotate(model, rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::rotate(model, rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
        
        model = glm::scale(model, scale);
        return model;
    }
};

class RoomMesh : public Mesh{
public:
    RoomMesh() {
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

class CubeMesh : public Mesh{
public:
    CubeMesh() {
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
            indices.push_back(baseIndex + 2);
            indices.push_back(baseIndex + 1);
            // 三角形2: 顶点0,2,3
            indices.push_back(baseIndex);
            indices.push_back(baseIndex + 3);
            indices.push_back(baseIndex + 2);
        }
    }
};

