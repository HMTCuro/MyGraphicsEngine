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
    void init(RendererContext* pCtx, BufferManager* pBufferManager){
        this->pCtx = pCtx;
        this->pBufferManager = pBufferManager;
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
        auto fpCreateAccelerationStructureKHR = (PFN_vkCreateAccelerationStructureKHR)vkGetDeviceProcAddr(pCtx->device, "vkCreateAccelerationStructureKHR");
        auto fpCmdBuildAccelerationStructuresKHR = (PFN_vkCmdBuildAccelerationStructuresKHR)vkGetDeviceProcAddr(pCtx->device, "vkCmdBuildAccelerationStructuresKHR");
        auto fpGetAccelerationStructureDeviceAddressKHR = (PFN_vkGetAccelerationStructureDeviceAddressKHR)vkGetDeviceProcAddr(pCtx->device, "vkGetAccelerationStructureDeviceAddressKHR");
        auto fpGetAccelerationStructureBuildSizesKHR = (PFN_vkGetAccelerationStructureBuildSizesKHR)vkGetDeviceProcAddr(pCtx->device, "vkGetAccelerationStructureBuildSizesKHR");

        VkAccelerationStructureBuildGeometryInfoKHR buildGeometryInfo = {};
        buildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
        buildGeometryInfo.type = asType;
        buildGeometryInfo.flags = flags;
        buildGeometryInfo.geometryCount = 1;
        buildGeometryInfo.pGeometries = &asGeometry;

        VkAccelerationStructureBuildSizesInfoKHR buildSizesInfo = {};
        buildSizesInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
        fpGetAccelerationStructureBuildSizesKHR(pCtx->device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &buildGeometryInfo, &asBuildRangeInfo.primitiveCount, &buildSizesInfo);

        pBufferManager->createBuffer(
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
        result = fpCreateAccelerationStructureKHR(pCtx->device, &createInfo, nullptr, &accelerationStructure.handle);
        if(result != VK_SUCCESS){
            throw std::runtime_error("failed to create acceleration structure!");
        }
        std::cout << "Acceleration Structure Created Successfully!" << std::endl;

        // Scratch buffer
        ScratchBuffer scratchBuffer;
        pBufferManager->createBuffer(
            buildSizesInfo.buildScratchSize,
            VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR|
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            scratchBuffer.handle,
            scratchBuffer.memory
        );
        scratchBuffer.deviceAddress = pBufferManager->getBufferDeviceAddress(scratchBuffer.handle);

        buildGeometryInfo.dstAccelerationStructure = accelerationStructure.handle;
        buildGeometryInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
        buildGeometryInfo.scratchData.deviceAddress = scratchBuffer.deviceAddress;

        VkCommandBuffer commandBuffer = pBufferManager->beginSingleTimeCommands();
        const VkAccelerationStructureBuildRangeInfoKHR* rangeInfoPtr = &asBuildRangeInfo;
        // result = fpCmdBuildAccelerationStructuresKHR(pCtx->device, nullptr, 1, &buildGeometryInfo, &rangeInfoPtr);
        fpCmdBuildAccelerationStructuresKHR(commandBuffer, 1, &buildGeometryInfo, &rangeInfoPtr);
        pBufferManager->waitIdle();
        pBufferManager->endSingleTimeCommands(commandBuffer);

        VkAccelerationStructureDeviceAddressInfoKHR asAddressInfo{};
        asAddressInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
        asAddressInfo.accelerationStructure = accelerationStructure.handle;
        accelerationStructure.deviceAddress = fpGetAccelerationStructureDeviceAddressKHR(pCtx->device, &asAddressInfo);

        vkDestroyBuffer(pCtx->device, scratchBuffer.handle, nullptr);
        vkFreeMemory(pCtx->device, scratchBuffer.memory, nullptr);

    }   
    VkAccelerationStructureInstanceKHR createTLASInstance(const glm::mat4& transform, uint32_t instanceCustomIndex, uint8_t mask, uint32_t sbtOffset, VkGeometryInstanceFlagsKHR flags,  AccelerationStructure& blas){

        auto toTransformMatrixKHR = [](const glm::mat4& m) {
            VkTransformMatrixKHR t;
            memcpy(&t, glm::value_ptr(glm::transpose(m)), sizeof(t));
            return t;
        };

        VkAccelerationStructureInstanceKHR instance{};
        instance.transform = toTransformMatrixKHR(transform);
        instance.instanceCustomIndex = instanceCustomIndex;
        instance.mask = mask;
        instance.accelerationStructureReference = blas.deviceAddress;
        instance.instanceShaderBindingTableRecordOffset = sbtOffset;
        instance.flags = flags;
        return instance;
    }
private:
    RendererContext* pCtx;
    BufferManager* pBufferManager;
};

class BottomLevelAS{
public:
    void init(Mesh* pMesh,RendererContext* pCtx, BufferManager* pBufferManager, AccelerationStructureManager* pAsManager){
        this->pMesh = pMesh;
        this->pCtx = pCtx;
        this->pBufferManager = pBufferManager;
        this->pAsManager = pAsManager;
    }
    void build(){
        std::cout << "--------Creating BLAS---------" << std::endl;

        VkResult result;

        // Bottom-level acceleration structure
        VkAccelerationStructureGeometryKHR geometry{};
        geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
        geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
        geometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
        geometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
        geometry.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
        geometry.geometry.triangles.vertexData.deviceAddress = pBufferManager->getBufferDeviceAddress(pMesh->vertexBuffer);
        geometry.geometry.triangles.vertexStride = sizeof(VertexN);
        geometry.geometry.triangles.maxVertex = static_cast<uint32_t>(pMesh->vertices.size());
        geometry.geometry.triangles.indexType = VK_INDEX_TYPE_UINT32;
        geometry.geometry.triangles.indexData.deviceAddress = pBufferManager->getBufferDeviceAddress(pMesh->indexBuffer);

        VkAccelerationStructureBuildRangeInfoKHR rangeInfo = {};
        rangeInfo.primitiveCount = pMesh->indices.size() / 3;
        rangeInfo.primitiveOffset = 0;
        rangeInfo.firstVertex = 0;
        rangeInfo.transformOffset = 0;

        pAsManager->createAccelerationStructure(
            VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,
            blas,
            geometry,
            rangeInfo,
            VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR
        );
    }
    void destroy(){
        auto fpDestroyAccelerationStructureKHR = (PFN_vkDestroyAccelerationStructureKHR)vkGetDeviceProcAddr(pCtx->device, "vkDestroyAccelerationStructureKHR");
        fpDestroyAccelerationStructureKHR(pCtx->device, blas.handle, nullptr);
        vkDestroyBuffer(pCtx->device, blas.buffer, nullptr);
        vkFreeMemory(pCtx->device, blas.memory, nullptr);
    }
    AccelerationStructure getHandle(){
        return blas;
    }
private:
    RendererContext* pCtx;
    AccelerationStructure blas;
    Mesh* pMesh;
    BufferManager* pBufferManager;
    AccelerationStructureManager* pAsManager;
};

class TopLevelAS{
public:
    glm::mat4 transform = glm::mat4(1.0f);
    void init(RendererContext* pCtx, BufferManager* pBufferManager, AccelerationStructureManager* pAsManager){
        this->pCtx = pCtx;
        this->pBufferManager = pBufferManager;
        this->pAsManager = pAsManager;
    }
    void build(std::vector<VkAccelerationStructureInstanceKHR>& instances){
        std::cout << "--------Creating TLAS---------" << std::endl;

        VkResult result;

        VkDeviceSize instanceBufferSize = instances.size() * sizeof(VkAccelerationStructureInstanceKHR);
        pBufferManager->createBuffer(
            instanceBufferSize,
            VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT|VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            tlasInstanceBuffer,
            tlasInstanceBufferMemory
        );

        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        pBufferManager->createBuffer(
            instanceBufferSize,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            stagingBuffer,
            stagingBufferMemory
        );

        void* data;
        vkMapMemory(pCtx->device, stagingBufferMemory, 0, instanceBufferSize, 0, &data);
        memcpy(data, instances.data(), instanceBufferSize);
        vkUnmapMemory(pCtx->device, stagingBufferMemory);

        pBufferManager->copyBuffer(stagingBuffer, tlasInstanceBuffer, instanceBufferSize);
        vkDestroyBuffer(pCtx->device, stagingBuffer, nullptr);
        vkFreeMemory(pCtx->device, stagingBufferMemory, nullptr);


        // Top-level acceleration structure
        VkAccelerationStructureGeometryInstancesDataKHR geometryInstances{};
        geometryInstances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
        geometryInstances.data.deviceAddress = pBufferManager->getBufferDeviceAddress(tlasInstanceBuffer);
        VkAccelerationStructureGeometryKHR geometry{};
        geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
        geometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
        geometry.geometry.instances = geometryInstances;
        VkAccelerationStructureBuildRangeInfoKHR rangeInfo = {};
        rangeInfo.primitiveCount = static_cast<uint32_t>(instances.size());
        pAsManager->createAccelerationStructure(
            VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR,
            tlas,
            geometry,
            rangeInfo,
            VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR
        );
        
    }
    void destroy(){
        auto fpDestroyAccelerationStructureKHR = (PFN_vkDestroyAccelerationStructureKHR)vkGetDeviceProcAddr(pCtx->device, "vkDestroyAccelerationStructureKHR");
        fpDestroyAccelerationStructureKHR(pCtx->device, tlas.handle, nullptr);
        vkDestroyBuffer(pCtx->device, tlas.buffer, nullptr);
        vkFreeMemory(pCtx->device, tlas.memory, nullptr);
        vkDestroyBuffer(pCtx->device, tlasInstanceBuffer, nullptr);
        vkFreeMemory(pCtx->device, tlasInstanceBufferMemory, nullptr);
    }
    AccelerationStructure getHandle(){
        return tlas;
    }
private:
    VkBuffer tlasInstanceBuffer;
    VkDeviceMemory tlasInstanceBufferMemory;
    RendererContext* pCtx;
    AccelerationStructure tlas;
    BufferManager* pBufferManager;
    AccelerationStructureManager* pAsManager;
};