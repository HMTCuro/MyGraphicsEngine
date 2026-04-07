#pragma once

#include <vulkan/vulkan.h>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE 
#define GLM_ENABLE_EXPERIMENTAL

#include "basicObjects.h"
#include "renderUtils.h"

struct AccelerationStructure
{
	VkAccelerationStructureKHR          handle;
	uint64_t                            deviceAddress;
	VkBuffer                            buffer;
    VkDeviceMemory                      memory;
};

class AccelerationStructureManager{
public:
    void getProperties(VkDevice device, BufferManager& bufferManager){
        this->device = device;
        this->bufferManager = bufferManager;
    }
    void createAccelerationStructure(
        VkAccelerationStructureTypeKHR asType,
        AccelerationStructure& accelerationStructure,
        VkAccelerationStructureGeometryKHR& asGeometry,
        VkAccelerationStructureBuildRangeInfoKHR& asBuildRangeInfo,
        VkBuildAccelerationStructureFlagsKHR flags
    ) {
        VkResult result;
        // 动态获取KHR扩展函数指针
        auto fpCreateAccelerationStructureKHR = (PFN_vkCreateAccelerationStructureKHR)vkGetDeviceProcAddr(device, "vkCreateAccelerationStructureKHR");
        auto fpCmdBuildAccelerationStructuresKHR = (PFN_vkCmdBuildAccelerationStructuresKHR)vkGetDeviceProcAddr(device, "vkCmdBuildAccelerationStructuresKHR");
        auto fpGetAccelerationStructureDeviceAddressKHR = (PFN_vkGetAccelerationStructureDeviceAddressKHR)vkGetDeviceProcAddr(device, "vkGetAccelerationStructureDeviceAddressKHR");
        auto fpGetAccelerationStructureBuildSizesKHR = (PFN_vkGetAccelerationStructureBuildSizesKHR)vkGetDeviceProcAddr(device, "vkGetAccelerationStructureBuildSizesKHR");

        VkAccelerationStructureBuildGeometryInfoKHR buildGeometryInfo = {};
        buildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
        buildGeometryInfo.type = asType;
        buildGeometryInfo.flags = flags;
        buildGeometryInfo.geometryCount = 1;
        buildGeometryInfo.pGeometries = &asGeometry;

        VkAccelerationStructureBuildSizesInfoKHR buildSizesInfo = {};
        buildSizesInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
        fpGetAccelerationStructureBuildSizesKHR(device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &buildGeometryInfo, &asBuildRangeInfo.primitiveCount, &buildSizesInfo);

        bufferManager.createBuffer(
            buildSizesInfo.accelerationStructureSize,
            VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            accelerationStructure.buffer,
            accelerationStructure.memory
        );
        std::cout<< "Acceleration Structure Buffer Created: Size = " << buildSizesInfo.accelerationStructureSize << " bytes" << std::endl;

        VkAccelerationStructureCreateInfoKHR createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
        createInfo.buffer = accelerationStructure.buffer;
        createInfo.size = buildSizesInfo.accelerationStructureSize;
        createInfo.type = asType;
        result = fpCreateAccelerationStructureKHR(device, &createInfo, nullptr, &accelerationStructure.handle);
        if(result != VK_SUCCESS){
            throw std::runtime_error("failed to create acceleration structure!");
        }
        std::cout << "Acceleration Structure Created Successfully!" << std::endl;

        // Scratch buffer
        ScratchBuffer scratchBuffer;
        bufferManager.createBuffer(
            buildSizesInfo.buildScratchSize,
            VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR|
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            scratchBuffer.handle,
            scratchBuffer.memory
        );
        scratchBuffer.deviceAddress = bufferManager.getBufferDeviceAddress(scratchBuffer.handle);

        buildGeometryInfo.dstAccelerationStructure = accelerationStructure.handle;
        buildGeometryInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
        buildGeometryInfo.scratchData.deviceAddress = scratchBuffer.deviceAddress;

        VkCommandBuffer commandBuffer = bufferManager.beginSingleTimeCommands();
        const VkAccelerationStructureBuildRangeInfoKHR* rangeInfoPtr = &asBuildRangeInfo;
        // result = fpBuildAccelerationStructuresKHR(device, nullptr, 1, &buildGeometryInfo, &rangeInfoPtr);
        fpCmdBuildAccelerationStructuresKHR(commandBuffer, 1, &buildGeometryInfo, &rangeInfoPtr);
        bufferManager.endSingleTimeCommands(commandBuffer);

        VkAccelerationStructureDeviceAddressInfoKHR asAddressInfo{};
        asAddressInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
        asAddressInfo.accelerationStructure = accelerationStructure.handle;
        accelerationStructure.deviceAddress = fpGetAccelerationStructureDeviceAddressKHR(device, &asAddressInfo);

        vkDestroyBuffer(device, scratchBuffer.handle, nullptr);
        vkFreeMemory(device, scratchBuffer.memory, nullptr);

    }   

private:
    VkDevice device;
    BufferManager bufferManager;
};

class BottomLevelAS{
public:
    void setProperties(Mesh* mesh, VkDevice device, BufferManager& bufferManager, AccelerationStructureManager& asManager){
        this->device = device;
        this->mesh = mesh;
        this->bufferManager = bufferManager;
        this->asManager = asManager;
    }
    void createBLAS(){
        std::cout << "--------Creating BLAS---------" << std::endl;

        VkResult result;

        // Bottom-level acceleration structure
        VkAccelerationStructureGeometryKHR geometry{};
        geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
        geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
        geometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
        geometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
        geometry.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
        geometry.geometry.triangles.vertexData.deviceAddress = bufferManager.getBufferDeviceAddress(mesh->vertexBuffer);
        geometry.geometry.triangles.vertexStride = sizeof(Vertex);
        geometry.geometry.triangles.maxVertex = static_cast<uint32_t>(mesh->vertices.size());
        geometry.geometry.triangles.indexType = VK_INDEX_TYPE_UINT32;
        geometry.geometry.triangles.indexData.deviceAddress = bufferManager.getBufferDeviceAddress(mesh->indexBuffer);

        VkAccelerationStructureBuildRangeInfoKHR rangeInfo = {};
        rangeInfo.primitiveCount = mesh->indices.size() / 3;
        rangeInfo.primitiveOffset = 0;
        rangeInfo.firstVertex = 0;
        rangeInfo.transformOffset = 0;

        asManager.createAccelerationStructure(
            VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,
            blas,
            geometry,
            rangeInfo,
            VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR
        );
    }
private:
    VkDevice device;
    AccelerationStructure blas;
    Mesh* mesh;
    BufferManager bufferManager;
    AccelerationStructureManager asManager;
};

class TopLevelAS{
public:
    glm::mat4 transform = glm::mat4(1.0f);
    void setProperties(VkDevice device, BufferManager& bufferManager, AccelerationStructureManager& asManager){
        this->device = device;
        this->bufferManager = bufferManager;
        this->asManager = asManager;
    }
   void createTLAS(){
    std::cout << "--------Creating TLAS---------" << std::endl;

    VkResult result;

    auto toTransformMatrixKHR = [](const glm::mat4& m) {
        VkTransformMatrixKHR t;
        memcpy(&t, glm::value_ptr(glm::transpose(m)), sizeof(t));
        return t;
    };

    std::vector<VkAccelerationStructureInstanceKHR> instances;
    
    VkAccelerationStructureInstanceKHR instance{};
    instance.transform = toTransformMatrixKHR(glm::mat4(1.0f)); // Identity
    instance.instanceCustomIndex = 0;
    instance.mask = 0xFF;
    instance.accelerationStructureReference = blas.deviceAddress;
    instance.instanceShaderBindingTableRecordOffset = 0; 
    instance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
    instances.push_back(instance);

    VkBuffer tlasInstanceBuffer;
    VkDeviceMemory tlasInstanceBufferMemory;
    VkDeviceSize instanceBufferSize = instances.size() * sizeof(VkAccelerationStructureInstanceKHR);
    bufferManager.createBuffer(
        instanceBufferSize,
        VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT|VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        tlasInstanceBuffer,
        tlasInstanceBufferMemory
    );

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    bufferManager.createBuffer(
        instanceBufferSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stagingBuffer,
        stagingBufferMemory
    );
    bufferManager.copyBuffer(stagingBuffer, tlasInstanceBuffer, instanceBufferSize);
    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);


    // Top-level acceleration structure
    VkAccelerationStructureGeometryInstancesDataKHR geometryInstances{};
    geometryInstances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
    geometryInstances.data.deviceAddress = bufferManager.getBufferDeviceAddress(tlasInstanceBuffer);
    VkAccelerationStructureGeometryKHR geometry{};
    geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
    geometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
    geometry.geometry.instances = geometryInstances;
    VkAccelerationStructureBuildRangeInfoKHR rangeInfo = {};
    rangeInfo.primitiveCount = static_cast<uint32_t>(instances.size());
    asManager.createAccelerationStructure(
        VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR,
        tlas,
        geometry,
        rangeInfo,
        VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR
    );
    
    vkDestroyBuffer(device, tlasInstanceBuffer, nullptr);
    vkFreeMemory(device, tlasInstanceBufferMemory, nullptr);
    
}
private:
    VkDevice device;
    AccelerationStructure tlas;
    AccelerationStructure blas;
    BufferManager bufferManager;
    AccelerationStructureManager asManager;
};