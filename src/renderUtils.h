#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE 
#define GLM_ENABLE_EXPERIMENTAL

#include "basicObjects.h"

#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>
#include <fstream>
#include <cstdint>

struct InstanceInfo{
    uint64_t  vertexBufferAddress;
    uint64_t  indexBufferAddress;
    glm::mat4 modelMatrix;
};

struct ScratchBuffer
{
	uint64_t       deviceAddress;
	VkBuffer       handle;
	VkDeviceMemory memory;
};

// Wraps all data required for an acceleration structure
class RendererContext{
public:
    VkDevice device= VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice= VK_NULL_HANDLE;
    VkInstance instance= VK_NULL_HANDLE;
    VkCommandPool commandPool= VK_NULL_HANDLE;
    VkQueue graphicsQueue= VK_NULL_HANDLE;
    uint32_t MAX_FRAMES_IN_FLIGHT = 2;
};

class WindowContext{
public:
    GLFWwindow* window = nullptr;
    uint32_t width = 800;
    uint32_t height = 600;  
};

class RenderUtils{
public:
    static uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties, VkPhysicalDevice physicalDevice);

    static void createCommandBuffers(RendererContext* pCtx, std::vector<VkCommandBuffer>& commandBuffers, uint32_t commandBufferCount);

    static VkDescriptorSetLayoutBinding createBinding(uint32_t binding, VkDescriptorType type, VkShaderStageFlags stageFlags, uint32_t descriptorCount = 1);

    static std::vector<VkDescriptorPoolSize> combineDescriptorPoolSizes(const std::vector<std::vector<VkDescriptorPoolSize>>& sizes);

    // 设置全屏视口和裁剪区域
    static void setViewportAndScissor(VkCommandBuffer commandBuffer, const VkExtent2D& extent);

    // 图像布局转换（封装 VkImageMemoryBarrier + vkCmdPipelineBarrier）
    static void transitionImageLayout(
        VkCommandBuffer commandBuffer,
        VkImage image,
        VkImageLayout oldLayout,
        VkImageLayout newLayout,
        VkAccessFlags srcAccessMask,
        VkAccessFlags dstAccessMask,
        VkPipelineStageFlags srcStage,
        VkPipelineStageFlags dstStage);

};

class BufferManager{
public:
    void init(RendererContext* ctx){
        this->pCtx = ctx;
    }
    
    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) {
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VkResult result = vkCreateBuffer(pCtx->device, &bufferInfo, nullptr, &buffer);
        if (result != VK_SUCCESS) {
            throw std::runtime_error("failed to create buffer!");
        }

        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(pCtx->device, buffer, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = RenderUtils::findMemoryType(memRequirements.memoryTypeBits, properties, pCtx->physicalDevice);

        if(usage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT){
            VkMemoryAllocateFlagsInfo flagsInfo{};
            flagsInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO;
            flagsInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT;
            allocInfo.pNext = &flagsInfo;
        }

        result = vkAllocateMemory(pCtx->device, &allocInfo, nullptr, &bufferMemory);
        if (result != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate buffer memory!");
        }

        vkBindBufferMemory(pCtx->device, buffer, bufferMemory, 0);
    }

    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size){
        VkCommandBuffer commandBuffer = beginSingleTimeCommands();
        VkBufferCopy copyRegion{};
        copyRegion.size = size;
        vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
        endSingleTimeCommands(commandBuffer);
    }

    VkCommandBuffer beginSingleTimeCommands(){
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = pCtx->commandPool;
        allocInfo.commandBufferCount = 1;

        VkCommandBuffer commandBuffer;
        vkAllocateCommandBuffers(pCtx->device, &allocInfo, &commandBuffer);

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        vkBeginCommandBuffer(commandBuffer, &beginInfo);

        return commandBuffer;
    }
    
    void endSingleTimeCommands(VkCommandBuffer commandBuffer){
        vkEndCommandBuffer(commandBuffer);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        vkQueueSubmit(pCtx->graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(pCtx->graphicsQueue);

        vkFreeCommandBuffers(pCtx->device, pCtx->commandPool, 1, &commandBuffer);
    }

    void waitIdle(){
        vkDeviceWaitIdle(pCtx->device);
    }

    void createStorageBuffer(VkDeviceSize bufferSize, StorageBuffer& StorageBuffer, void* data){
        StorageBuffer.buffer.resize(pCtx->MAX_FRAMES_IN_FLIGHT);
        StorageBuffer.bufferMemory.resize(pCtx->MAX_FRAMES_IN_FLIGHT);
        StorageBuffer.bufferMapped.resize(pCtx->MAX_FRAMES_IN_FLIGHT);

        for (size_t i = 0; i < pCtx->MAX_FRAMES_IN_FLIGHT; i++) {
            createBuffer(bufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, StorageBuffer.buffer[i], StorageBuffer.bufferMemory[i]);

            void* mappedData;
            vkMapMemory(pCtx->device, StorageBuffer.bufferMemory[i], 0, bufferSize, 0, &mappedData);
            memcpy(mappedData, data, bufferSize);
            vkUnmapMemory(pCtx->device, StorageBuffer.bufferMemory[i]);
        }
    }

    void createMeshVertexBuffer(Mesh* object) { 
        VkDeviceSize bufferSize = sizeof(object->vertices[0]) * object->vertices.size();

        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;

        createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

        void* data;
        vkMapMemory(pCtx->device, stagingBufferMemory, 0, bufferSize, 0, &data);
            memcpy(data, object->vertices.data(), (size_t) bufferSize);
        vkUnmapMemory(pCtx->device, stagingBufferMemory);
        createBuffer(
            bufferSize, 
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR, 
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            object->vertexBuffer, object->vertexBufferMemory);

        copyBuffer(stagingBuffer, object->vertexBuffer, bufferSize);
        vkDestroyBuffer(pCtx->device, stagingBuffer, nullptr);
        vkFreeMemory(pCtx->device, stagingBufferMemory, nullptr);
    }

    void createMeshIndexBuffer(Mesh* object) { 
        VkDeviceSize bufferSize = sizeof(object->indices[0]) * object->indices.size();

        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;

        createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

        void* data;
        vkMapMemory(pCtx->device, stagingBufferMemory, 0, bufferSize, 0, &data);
            memcpy(data, object->indices.data(), (size_t) bufferSize);
        vkUnmapMemory(pCtx->device, stagingBufferMemory);

        createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT| VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, object->indexBuffer, object->indexBufferMemory);

        copyBuffer(stagingBuffer, object->indexBuffer, bufferSize);

        vkDestroyBuffer(pCtx->device, stagingBuffer, nullptr);
        vkFreeMemory(pCtx->device, stagingBufferMemory, nullptr);

    }
   
    void createUniformBuffers(VkDeviceSize bufferSize, UniformBuffer& uniformBuffer) {
        uniformBuffer.buffer.resize(pCtx->MAX_FRAMES_IN_FLIGHT);
        uniformBuffer.bufferMemory.resize(pCtx->MAX_FRAMES_IN_FLIGHT);
        uniformBuffer.bufferMapped.resize(pCtx->MAX_FRAMES_IN_FLIGHT);

        for (size_t i = 0; i < pCtx->MAX_FRAMES_IN_FLIGHT; i++) {
            createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniformBuffer.buffer[i], uniformBuffer.bufferMemory[i]);

            vkMapMemory(pCtx->device, uniformBuffer.bufferMemory[i], 0, bufferSize, 0, &uniformBuffer.bufferMapped[i]);
        }
    }

    void updateUniformBuffer(uint32_t currentImage, UniformBuffer& uniformBuffer, void* data, size_t dataSize) {
        memcpy(uniformBuffer.bufferMapped[currentImage], data, dataSize);
    }

    void destroyUniformBuffers(UniformBuffer& uniformBuffer) {
        for (size_t i = 0; i < pCtx->MAX_FRAMES_IN_FLIGHT; i++) {
            vkUnmapMemory(pCtx->device, uniformBuffer.bufferMemory[i]);
            vkDestroyBuffer(pCtx->device, uniformBuffer.buffer[i], nullptr);
            vkFreeMemory(pCtx->device, uniformBuffer.bufferMemory[i], nullptr);
        }
    }

    void destroyStorageBuffers(StorageBuffer& storageBuffer) {
        for (size_t i = 0; i < pCtx->MAX_FRAMES_IN_FLIGHT; i++) {
            vkDestroyBuffer(pCtx->device, storageBuffer.buffer[i], nullptr);
            vkFreeMemory(pCtx->device, storageBuffer.bufferMemory[i], nullptr);
        }
    }
    
    VkDeviceAddress getBufferDeviceAddress(VkBuffer buffer){ 
        VkBufferDeviceAddressInfo bufferDeviceAddressInfo{};
        bufferDeviceAddressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
        bufferDeviceAddressInfo.buffer = buffer;
        auto fpGetBufferDeviceAddress = (PFN_vkGetBufferDeviceAddressKHR) vkGetDeviceProcAddr(pCtx->device, "vkGetBufferDeviceAddressKHR");
        VkDeviceAddress deviceAddress = fpGetBufferDeviceAddress(pCtx->device, &bufferDeviceAddressInfo);
        return deviceAddress;
    }
private:
    RendererContext* pCtx;
};

class ImageFactory{
public:
    static void createImage(
        uint32_t width, uint32_t height, uint32_t mipLevels, 
        VkSampleCountFlagBits numSamples, VkFormat format, 
        VkImageTiling tiling, VkImageUsageFlags usage, 
        VkMemoryPropertyFlags properties, VkImage& image, 
        VkDeviceMemory& imageMemory, 
        VkPhysicalDevice physicalDevice, VkDevice device);

    static VkImageView createImageView(VkImage image, VkFormat format,VkImageAspectFlags aspectFlags, uint32_t miplevels, VkDevice device);

};

class DescriptorWriteCollector {
public:
    // 添加一个 Buffer 绑定
    void addBuffer(VkDescriptorSet dstSet,
                   uint32_t binding,
                   VkDescriptorType type,         // VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER / STORAGE_BUFFER
                   VkBuffer buffer,
                   VkDeviceSize offset,
                   VkDeviceSize range,
                   uint32_t arrayElement = 0);
    // 添加一个 Image 绑定
    void addImage(VkDescriptorSet dstSet,
                  uint32_t binding,
                  VkDescriptorType type,         // VK_DESCRIPTOR_TYPE_STORAGE_IMAGE / COMBINED_IMAGE_SAMPLER
                  VkImageView imageView,
                  VkImageLayout imageLayout,
                  VkSampler sampler = VK_NULL_HANDLE,
                  uint32_t arrayElement = 0);

    // 添加一组 Acceleration Structure 绑定（通常对 TLAS）
    void addAccelerationStructures(VkDescriptorSet dstSet,
                                   uint32_t binding,
                                   const std::vector<VkAccelerationStructureKHR>& handles,
                                   uint32_t arrayElement = 0);

    // 单 TLAS 的便捷重载
    void addAccelerationStructure(VkDescriptorSet dstSet,
                                  uint32_t binding,
                                  VkAccelerationStructureKHR tlas,
                                  uint32_t arrayElement = 0)
    {
        addAccelerationStructures(dstSet, binding, {tlas}, arrayElement);
    }

    // 提交所有累积的写入（先修复指针，再调用 vkUpdateDescriptorSets）
    void update(VkDevice device);

    // 清空，以便复用于下一帧（可选）
    void reset();

private:
    std::vector<VkWriteDescriptorSet> writes;
    std::vector<VkDescriptorBufferInfo> bufferInfos;
    std::vector<VkDescriptorImageInfo> imageInfos;
    std::vector<VkWriteDescriptorSetAccelerationStructureKHR> asExtensions;
    std::vector<VkAccelerationStructureKHR> asHandles;

    // 修复信息：add* 时只存索引，update() 时 vector 已稳定，再计算最终指针
    enum class InfoType { NONE, BUFFER, IMAGE, AS_EXT };
    struct WriteFixup {
        InfoType type{InfoType::NONE};
        size_t index{0};
        size_t asHandlesOffset{0};  // 仅 AS_EXT：在 asHandles 中的偏移
    };
    std::vector<WriteFixup> writeFixups;
};


// class DescriptorPool{
// public:
//     VkDescriptorPool getHandle(){
//         return descriptorPool;
//     }

//     void destroy(){
//         vkDestroyDescriptorPool(pCtx->device, descriptorPool, nullptr);
//     }
// private:
//     RendererContext* pCtx;
//     VkDescriptorPool descriptorPool;
// };

// BufferManager Class





