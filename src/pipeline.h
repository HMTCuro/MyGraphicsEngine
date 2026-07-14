#pragma once

// #define SAMPLER_TEST

#include <vulkan/vulkan.h>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE 
#define GLM_ENABLE_EXPERIMENTAL

#include "basicObjects.h"
#include "renderUtils.h"
#include "shaderUtils.h"

#include <fstream>
#include <unordered_map>


const std::string vshPaths[] = {
    "../shader_spv/vert.spv",
    "../shader_spv/sampler_vert.spv"
};

const std::string fshPaths[] = {
    "../shader_spv/frag.spv",
    "../shader_spv/sampler_frag.spv"
};

const std::string cshPaths[] = {
    "../shader_spv/comp.spv"
};

// Implementation of ray tracing descriptor set layout creation goes here
// set 0 binding 0: tlas
// set 0 binding 1: output image
// set 0 binding 2: instance info buffer
// set 1 binding 0: global uniform buffer (window size, clear color, scale)
// set 1 binding 1: camera uniform buffer
// set 1 binding 2: light uniform buffer
const std::vector<std::vector<VkDescriptorSetLayoutBinding>> rayTracingDescriptorSetLayoutConfigs = {
    {
        RenderUtils::createBinding(
            0, 
            VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 
            VK_SHADER_STAGE_ALL
        ),
        RenderUtils::createBinding(
            1, 
            VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 
            VK_SHADER_STAGE_ALL
        ),
        RenderUtils::createBinding(
            2, 
            VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 
            VK_SHADER_STAGE_ALL
        )
    },
    {
        RenderUtils::createBinding(
            0, 
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 
            VK_SHADER_STAGE_ALL
        ),
        RenderUtils::createBinding(
            1, 
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 
            VK_SHADER_STAGE_ALL
        ),
        RenderUtils::createBinding(
            2, 
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 
            VK_SHADER_STAGE_ALL
        )
    },
    {
        RenderUtils::createBinding(
            0, 
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 
            VK_SHADER_STAGE_ALL
        ), //Texture info: index,...
        RenderUtils::createBinding(
            1, 
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            VK_SHADER_STAGE_ALL,
            2
        ), //Texture data
    },
    {
        RenderUtils::createBinding(
            0, 
            VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 
            VK_SHADER_STAGE_ALL
        ),
        RenderUtils::createBinding(
            1, 
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 
            VK_SHADER_STAGE_ALL
        ),
        RenderUtils::createBinding(
            2, 
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 
            VK_SHADER_STAGE_ALL
        )
    }
};

#ifndef SAMPLER_TEST
const std::vector<std::vector<VkDescriptorSetLayoutBinding>> samplerDescriptorSetLayoutConfigs = {
    {
        RenderUtils::createBinding(
            0,
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            VK_SHADER_STAGE_ALL
        )
    }
};
#else
const std::vector<std::vector<VkDescriptorSetLayoutBinding>> samplerDescriptorSetLayoutConfigs = {
    {
        RenderUtils::createBinding(
            0,
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            VK_SHADER_STAGE_ALL
        ),
        RenderUtils::createBinding(
            1,
            VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            VK_SHADER_STAGE_ALL
        ),
        RenderUtils::createBinding(
            2,
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            VK_SHADER_STAGE_ALL
        ),
        RenderUtils::createBinding(
            3,
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            VK_SHADER_STAGE_ALL
        )
    }
};
#endif

const std::vector<std::vector<VkDescriptorSetLayoutBinding>> whiteModelDescriptorSetLayoutConfigs = {
    {
        RenderUtils::createBinding(
            0, 
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 
            VK_SHADER_STAGE_ALL
        ),
        RenderUtils::createBinding(
            1, 
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 
            VK_SHADER_STAGE_ALL
        )
    }
};


class BasePipeline{
public:
    uint32_t MAX_FRAME_IN_FLIGHT = 2;
    VkPipeline getHandle(){
        return pipeline;
    }
    VkPipelineLayout getLayout(){
        return pipelineLayout;
    }
    void setProperties(
        RendererContext* pCtx, VkRenderPass renderPass, 
        const std::vector<VkDescriptorSetLayout>& descriptorSetLayouts, BufferManager* bufferManager){
        this->pCtx = pCtx;
        this->renderPass = renderPass;
        this->descriptorSetLayouts = descriptorSetLayouts;
        this->bufferManager = bufferManager;

    }
    virtual void createPipeline() = 0;
    void destroyPipeline() {
        vkDestroyPipeline(pCtx->device, pipeline, nullptr);
        vkDestroyPipelineLayout(pCtx->device, pipelineLayout, nullptr);
    }
protected:
    RendererContext* pCtx;
    VkRenderPass renderPass = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout;
    VkPipeline pipeline;
    std::vector<VkDescriptorSetLayout> descriptorSetLayouts;
    BufferManager* bufferManager;

};

class DescriptorSetLayoutBundle{
public:
    std::vector<VkDescriptorSetLayout> getHandle(){
        return descriptorSetLayouts;
    }
    std::vector<VkDescriptorPoolSize> getPoolSizes(){
        return poolSizes;
    }
    uint32_t getCount(){
        return static_cast<uint32_t>(descriptorSetLayouts.size());
    }
    void setProperties(VkDevice device){
        this->device = device;
    }

    void createDescriptorSetLayoutFromConfig(const std::vector<std::vector<VkDescriptorSetLayoutBinding>>& layoutConfigs){
        for(const auto& bindings : layoutConfigs){
            addLayout(bindings);
        }
    }

    void destroyDescriptorSetLayout() {
        for (const auto& descriptorSetLayout : descriptorSetLayouts) {
            vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
        }
    }
protected:
    VkDevice device;
    std::vector<VkDescriptorSetLayout> descriptorSetLayouts;
    std::vector<VkDescriptorPoolSize> poolSizes;

    void addLayout(const std::vector<VkDescriptorSetLayoutBinding>& bindings){
        std::vector<VkDescriptorSetLayoutBinding> layoutBindings = bindings;
        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = static_cast<uint32_t>(layoutBindings.size());
        layoutInfo.pBindings = layoutBindings.data();

        VkDescriptorSetLayout descriptorSetLayout;
        VkResult result = vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout);
        if (result != VK_SUCCESS) {
            throw std::runtime_error("failed to create descriptor set layout!");
        }
        descriptorSetLayouts.push_back(descriptorSetLayout);

        for (const auto& binding : layoutBindings) {
            updatePoolSizes(binding);
        }
    }

    void updatePoolSizes(const VkDescriptorSetLayoutBinding& binding){
        for (auto& poolSize : poolSizes) {
            if (poolSize.type == binding.descriptorType) {
                poolSize.descriptorCount += binding.descriptorCount;
                return;
            }
        }
        VkDescriptorPoolSize newPoolSize{};
        newPoolSize.type = binding.descriptorType;
        newPoolSize.descriptorCount = binding.descriptorCount;
        poolSizes.push_back(newPoolSize);
    }
};

class WhiteModelPipeline1: public BasePipeline{
public:
    void createPipeline() override {
        VkShaderModule vertShaderModule=ShaderFactory::CreateShaderModuleFromFile(pCtx->device, vshPaths[0]);
        VkShaderModule fragShaderModule=ShaderFactory::CreateShaderModuleFromFile(pCtx->device, fshPaths[0]);

        VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
        vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertShaderStageInfo.module = vertShaderModule;
        vertShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
        fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragShaderStageInfo.module = fragShaderModule;
        fragShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

        auto bindingDescription = VertexN::getBindingDescription();
        auto attributeDescriptions = VertexN::getAttributeDescriptions();

        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexBindingDescriptionCount = 1;
        vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
        vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
        vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.scissorCount = 1;

        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
        rasterizer.depthBiasEnable = VK_FALSE;

        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        VkPipelineDepthStencilStateCreateInfo depthStencil{};
        depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencil.depthTestEnable = VK_TRUE;
        depthStencil.depthWriteEnable = VK_TRUE;
        depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
        depthStencil.depthBoundsTestEnable = VK_FALSE;
        depthStencil.stencilTestEnable = VK_FALSE;

        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_FALSE;

        VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;
        colorBlending.blendConstants[0] = 0.0f;
        colorBlending.blendConstants[1] = 0.0f;
        colorBlending.blendConstants[2] = 0.0f;
        colorBlending.blendConstants[3] = 0.0f;

        std::vector<VkDynamicState> dynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        };

        VkPipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
        dynamicState.pDynamicStates = dynamicStates.data();

        VkPushConstantRange pushConstantRange{}; // mat4
        pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        pushConstantRange.offset = 0;
        pushConstantRange.size = sizeof(glm::mat4);

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
        pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
        pipelineLayoutInfo.pushConstantRangeCount = 1;
        pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

        VkResult result =vkCreatePipelineLayout(pCtx->device, &pipelineLayoutInfo, nullptr, &pipelineLayout);
        if(result != VK_SUCCESS){
            throw std::runtime_error("failed to create pipeline layout!");
        }
        

        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = shaderStages;
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pDepthStencilState = &depthStencil;
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.pDynamicState = &dynamicState;
        pipelineInfo.layout = pipelineLayout;
        pipelineInfo.renderPass = renderPass;
        pipelineInfo.subpass = 0;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

        VkResult result1 = vkCreateGraphicsPipelines(pCtx->device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline);
        if (result1 != VK_SUCCESS) {
            throw std::runtime_error("failed to create graphics pipeline!");
        }

        vkDestroyShaderModule(pCtx->device, fragShaderModule, nullptr);
        vkDestroyShaderModule(pCtx->device, vertShaderModule, nullptr);
    }
};

class RayTracingPipeline: public BasePipeline{
public:
    VkBuffer sbtBuffer;
    VkDeviceMemory sbtBufferMemory;
    uint8_t* sbtMapping;

    VkStridedDeviceAddressRegionKHR raygenRegion{};
    VkStridedDeviceAddressRegionKHR missRegion{};
    VkStridedDeviceAddressRegionKHR hitRegion{};
    VkStridedDeviceAddressRegionKHR callableRegion{};

    
    void createPipeline() override {
        // Implementation of ray tracing pipeline creation goes here
        std::cout << "--------Creating Ray Tracing Pipeline---------" << std::endl;
        VkResult result;
        enum StageIndices {
            eRaygen,
            eMiss,
            eClosestHit,
            eLetherChit,
            eShaderGroupCount
        };
        std::array<VkShaderModule,eShaderGroupCount> shaderModules = {
            ShaderFactory::CreateShaderModuleFromFile(pCtx->device, "../shader_spv/raygen.spv"),
            ShaderFactory::CreateShaderModuleFromFile(pCtx->device, "../shader_spv/miss.spv"),
            ShaderFactory::CreateShaderModuleFromFile(pCtx->device, "../shader_spv/closeHit.spv"),
            ShaderFactory::CreateShaderModuleFromFile(pCtx->device, "../shader_spv/leather.spv")
        };

        std::array<VkPipelineShaderStageCreateInfo, eShaderGroupCount> shaderStages = { 
            ShaderFactory::CreateShaderStageInfo(
                VK_SHADER_STAGE_RAYGEN_BIT_KHR, 
                shaderModules[eRaygen], 
                "main"),
            ShaderFactory::CreateShaderStageInfo(
                VK_SHADER_STAGE_MISS_BIT_KHR,
                shaderModules[eMiss],
                "main"),
            ShaderFactory::CreateShaderStageInfo(
                VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR,
                shaderModules[eClosestHit],
                "main"),
            ShaderFactory::CreateShaderStageInfo(
                VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR,
                shaderModules[eLetherChit],
                "main")
        };

        std::vector<VkRayTracingShaderGroupCreateInfoKHR> shaderGroups={
            ShaderFactory::CreateRTShaderGroup(
                VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR,
                eRaygen, 
                VK_SHADER_UNUSED_KHR, 
                VK_SHADER_UNUSED_KHR, 
                VK_SHADER_UNUSED_KHR),
            ShaderFactory::CreateRTShaderGroup(
                VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR,
                eMiss, 
                VK_SHADER_UNUSED_KHR, 
                VK_SHADER_UNUSED_KHR, 
                VK_SHADER_UNUSED_KHR),
            ShaderFactory::CreateRTShaderGroup(
                VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR,
                VK_SHADER_UNUSED_KHR, 
                eClosestHit, 
                VK_SHADER_UNUSED_KHR, 
                VK_SHADER_UNUSED_KHR),
            ShaderFactory::CreateRTShaderGroup(
                VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR,
                VK_SHADER_UNUSED_KHR, 
                eLetherChit,
                VK_SHADER_UNUSED_KHR,
                VK_SHADER_UNUSED_KHR)

        };

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
        pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
        result = vkCreatePipelineLayout(pCtx->device, &pipelineLayoutInfo, nullptr, &pipelineLayout);
        if(result != VK_SUCCESS){
            throw std::runtime_error("failed to create pipeline layout!");
        }

        VkRayTracingPipelineCreateInfoKHR rtPipelineInfo{};
        rtPipelineInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
        rtPipelineInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
        rtPipelineInfo.pStages = shaderStages.data();
        rtPipelineInfo.groupCount = static_cast<uint32_t>(shaderGroups.size());
        rtPipelineInfo.pGroups = shaderGroups.data();
        rtPipelineInfo.layout = pipelineLayout;
        rtPipelineInfo.maxPipelineRayRecursionDepth = 2;

        auto fpCreateRayTracingPipelinesKHR = (PFN_vkCreateRayTracingPipelinesKHR)vkGetDeviceProcAddr(pCtx->device, "vkCreateRayTracingPipelinesKHR");
        result = fpCreateRayTracingPipelinesKHR(pCtx->device, VK_NULL_HANDLE, VK_NULL_HANDLE, 1, &rtPipelineInfo, nullptr, &pipeline);
        if(result != VK_SUCCESS){
            throw std::runtime_error("failed to create ray tracing pipeline!");
        }

        for (const auto& shaderModule : shaderModules) {
            vkDestroyShaderModule(pCtx->device, shaderModule, nullptr);
        }

        createShaderBindingTable(rtPipelineInfo);
    }
private:
    void createShaderBindingTable(const VkRayTracingPipelineCreateInfoKHR& rtPipelineInfo) {
        VkResult result;
        VkPhysicalDeviceRayTracingPipelinePropertiesKHR rtProperties{};
        rtProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;
        VkPhysicalDeviceAccelerationStructurePropertiesKHR asProperties{};
        asProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_PROPERTIES_KHR;

        VkPhysicalDeviceProperties2 deviceProperties2{};
        deviceProperties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
        deviceProperties2.pNext = &rtProperties;
        rtProperties.pNext = &asProperties;
        vkGetPhysicalDeviceProperties2(pCtx->physicalDevice, &deviceProperties2);
        
        // Ray Tracing Pipeline Properties:
        // Shader Group Handle Size: 32 bytes
        // Max Ray Recursion Depth: 31
        // Max Shader Group Stride: 4096 bytes
        // Shader Group Base Alignment: 64 bytes
        // Shader Group Handle Capture Replay Size: 64 bytes
        // Max Ray Dispatch Invocation Count: 1073741824
        // Shader Group Handle Alignment: 32 bytes
        // Max Ray Hit Attribute Size: 32 bytes
        
        uint32_t handleSize      = rtProperties.shaderGroupHandleSize;
        uint32_t handleAlignment = rtProperties.shaderGroupHandleAlignment;
        uint32_t baseAlignment   = rtProperties.shaderGroupBaseAlignment;
        uint32_t groupCount      = rtPipelineInfo.groupCount;

        // Get shader group handles
        size_t dataSize = handleSize * groupCount;
        std::vector<uint8_t> shaderHandles(dataSize);
        auto fpGetRayTracingShaderGroupHandlesKHR = (PFN_vkGetRayTracingShaderGroupHandlesKHR)vkGetDeviceProcAddr(pCtx->device, "vkGetRayTracingShaderGroupHandlesKHR");
        result = fpGetRayTracingShaderGroupHandlesKHR(pCtx->device, pipeline, 0, groupCount, dataSize, shaderHandles.data());
        if(result != VK_SUCCESS){
            throw std::runtime_error("failed to get ray tracing shader group handles!");
        }

        // Calculate SBT buffer size with proper alignment
        auto alignUp = [](uint32_t size, uint32_t alignment) { 
            return (size + alignment - 1) & ~(alignment - 1);
        };


        uint32_t raygenSize     = alignUp(handleSize, handleAlignment);
        uint32_t missSize       = alignUp(handleSize, handleAlignment);
        uint32_t hitHandleSize  = alignUp(handleSize, handleAlignment);   // One shader group handle per entry
        uint32_t hitStride      = hitHandleSize;                          // Stride between hit group entries
        uint32_t hitSize        = hitStride * 2;                          // Two hit groups
        uint32_t callableSize   = 0;  // No callable shaders in this tutorial

        // Ensure each region starts at a baseAlignment boundary
        uint32_t raygenOffset   = 0;
        uint32_t missOffset     = alignUp(raygenSize, baseAlignment);
        uint32_t hitOffset      = alignUp(missOffset + missSize, baseAlignment);
        uint32_t callableOffset = alignUp(hitOffset + hitSize, baseAlignment);

        size_t bufferSize = callableOffset + callableSize;

        bufferManager->createBuffer(
            bufferSize, 
            VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, 
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
            sbtBuffer, sbtBufferMemory);

        vkMapMemory(pCtx->device, sbtBufferMemory, 0, bufferSize, 0, (void**)&sbtMapping);

        // Populate SBT buffer
        uint8_t* pData = static_cast<uint8_t*>(sbtMapping);

        // Ray generation shader (group 0)
        
        memcpy(pData + raygenOffset, shaderHandles.data() + 0 * handleSize, handleSize);
        raygenRegion.deviceAddress = bufferManager->getBufferDeviceAddress(sbtBuffer) + raygenOffset;
        raygenRegion.stride        = raygenSize;
        raygenRegion.size          = raygenSize;


        // Miss shader (group 1)
        memcpy(pData + missOffset, shaderHandles.data() + 1 * handleSize, handleSize);
        missRegion.deviceAddress = bufferManager->getBufferDeviceAddress(sbtBuffer) + missOffset;
        missRegion.stride        = missSize;
        missRegion.size          = missSize;


        // Hit shader group 2 (closeHit)
        memcpy(pData + hitOffset, shaderHandles.data() + 2 * handleSize, handleSize);
        // Hit shader group 3 (leather)
        memcpy(pData + hitOffset + hitStride, shaderHandles.data() + 3 * handleSize, handleSize);
        hitRegion.deviceAddress = bufferManager->getBufferDeviceAddress(sbtBuffer) + hitOffset;
        hitRegion.stride        = hitStride;
        hitRegion.size          = hitSize;

        // Callable shaders (none in this tutorial)
        callableRegion.deviceAddress = 0;
        callableRegion.stride        = 0;
        callableRegion.size          = 0;

    // LOGI("Shader binding table created and populated \n");
        
    }
};

#ifndef SAMPLER_TEST
class TextureSamplerPipeline: public BasePipeline{
public:
    void createPipeline() override {
        // 1. 读取全屏顶点和片元着色器
        VkShaderModule vertShaderModule=ShaderFactory::CreateShaderModuleFromFile(pCtx->device, vshPaths[1]);
        VkShaderModule fragShaderModule=ShaderFactory::CreateShaderModuleFromFile(pCtx->device, fshPaths[1]);

        VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
        vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertShaderStageInfo.module = vertShaderModule;
        vertShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
        fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragShaderStageInfo.module = fragShaderModule;
        fragShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

        // 2. 顶点输入：全屏quad可以不需要顶点buffer，直接在shader里生成顶点
        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexBindingDescriptionCount = 0;
        vertexInputInfo.pVertexBindingDescriptions = nullptr;
        vertexInputInfo.vertexAttributeDescriptionCount = 0;
        vertexInputInfo.pVertexAttributeDescriptions = nullptr;

        // 3. 装配方式：三角形
        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        // 4. 视口和裁剪
        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.scissorCount = 1;

        // 5. 光栅化：正面朝外，关闭背面剔除（全屏quad不需要剔除）
        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VK_CULL_MODE_NONE;
        rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
        rasterizer.depthBiasEnable = VK_FALSE;

        // 6. 关闭多重采样
        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        // 7. 关闭深度测试
        VkPipelineDepthStencilStateCreateInfo depthStencil{};
        depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencil.depthTestEnable = VK_FALSE;
        depthStencil.depthWriteEnable = VK_FALSE;
        depthStencil.depthCompareOp = VK_COMPARE_OP_ALWAYS;
        depthStencil.depthBoundsTestEnable = VK_FALSE;
        depthStencil.stencilTestEnable = VK_FALSE;

        // 8. 颜色混合
        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_FALSE;

        VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;
        colorBlending.blendConstants[0] = 0.0f;
        colorBlending.blendConstants[1] = 0.0f;
        colorBlending.blendConstants[2] = 0.0f;
        colorBlending.blendConstants[3] = 0.0f;

        // 9. 动态状态
        std::vector<VkDynamicState> dynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        };

        VkPipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
        dynamicState.pDynamicStates = dynamicStates.data();

        // 10. 管线布局
        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
        pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();

        VkResult result = vkCreatePipelineLayout(pCtx->device, &pipelineLayoutInfo, nullptr, &pipelineLayout);
        if(result != VK_SUCCESS){
            throw std::runtime_error("failed to create pipeline layout!");
        }

        // 11. 创建图形管线
        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = shaderStages;
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pDepthStencilState = &depthStencil;
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.pDynamicState = &dynamicState;
        pipelineInfo.layout = pipelineLayout;
        pipelineInfo.renderPass = renderPass;
        pipelineInfo.subpass = 0;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

        VkResult result1 = vkCreateGraphicsPipelines(pCtx->device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline);
        if (result1 != VK_SUCCESS) {
            throw std::runtime_error("failed to create graphics pipeline!");
        }

        vkDestroyShaderModule(pCtx->device, fragShaderModule, nullptr);
        vkDestroyShaderModule(pCtx->device, vertShaderModule, nullptr);
    }
};
#else
class TextureSamplerPipeline: public BasePipeline{
public:
    void createPipeline() override {
        // 1. 读取全屏顶点和片元着色器
        VkShaderModule vertShaderModule=ShaderFactory::CreateShaderModuleFromFile(pCtx->device, vshPaths[1]);
        VkShaderModule fragShaderModule=ShaderFactory::CreateShaderModuleFromFile(pCtx->device, fshPaths[1]);

        VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
        vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertShaderStageInfo.module = vertShaderModule;
        vertShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
        fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragShaderStageInfo.module = fragShaderModule;
        fragShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

        // 2. 顶点输入：全屏quad可以不需要顶点buffer，直接在shader里生成顶点
        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexBindingDescriptionCount = 0;
        vertexInputInfo.pVertexBindingDescriptions = nullptr;
        vertexInputInfo.vertexAttributeDescriptionCount = 0;
        vertexInputInfo.pVertexAttributeDescriptions = nullptr;

        // 3. 装配方式：三角形
        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        // 4. 视口和裁剪
        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.scissorCount = 1;

        // 5. 光栅化：正面朝外，关闭背面剔除（全屏quad不需要剔除）
        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VK_CULL_MODE_NONE;
        rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
        rasterizer.depthBiasEnable = VK_FALSE;

        // 6. 关闭多重采样
        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        // 7. 关闭深度测试
        VkPipelineDepthStencilStateCreateInfo depthStencil{};
        depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencil.depthTestEnable = VK_FALSE;
        depthStencil.depthWriteEnable = VK_FALSE;
        depthStencil.depthCompareOp = VK_COMPARE_OP_ALWAYS;
        depthStencil.depthBoundsTestEnable = VK_FALSE;
        depthStencil.stencilTestEnable = VK_FALSE;

        // 8. 颜色混合
        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_FALSE;

        VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;
        colorBlending.blendConstants[0] = 0.0f;
        colorBlending.blendConstants[1] = 0.0f;
        colorBlending.blendConstants[2] = 0.0f;
        colorBlending.blendConstants[3] = 0.0f;

        // 9. 动态状态
        std::vector<VkDynamicState> dynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        };

        VkPipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
        dynamicState.pDynamicStates = dynamicStates.data();

        // 10. 管线布局
        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
        pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();

        VkResult result = vkCreatePipelineLayout(pCtx->device, &pipelineLayoutInfo, nullptr, &pipelineLayout);
        if(result != VK_SUCCESS){
            throw std::runtime_error("failed to create pipeline layout!");
        }

        // 11. 创建图形管线
        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = shaderStages;
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pDepthStencilState = &depthStencil;
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.pDynamicState = &dynamicState;
        pipelineInfo.layout = pipelineLayout;
        pipelineInfo.renderPass = renderPass;
        pipelineInfo.subpass = 0;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

        VkResult result1 = vkCreateGraphicsPipelines(pCtx->device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline);
        if (result1 != VK_SUCCESS) {
            throw std::runtime_error("failed to create graphics pipeline!");
        }

        vkDestroyShaderModule(pCtx->device, fragShaderModule, nullptr);
        vkDestroyShaderModule(pCtx->device, vertShaderModule, nullptr);
    }
};
#endif


class ComputePipeline: public BasePipeline{
public:
    void createPipeline() override {
        // Implementation of compute pipeline creation goes here
        VkResult result;
        VkShaderModule computeShaderModule = ShaderFactory::CreateShaderModuleFromFile(pCtx->device, cshPaths[0]);

        VkPipelineShaderStageCreateInfo computeShaderStageInfo{};
        computeShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        computeShaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
        computeShaderStageInfo.module = computeShaderModule;
        computeShaderStageInfo.pName = "main";

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
        pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();

        result = vkCreatePipelineLayout(pCtx->device, &pipelineLayoutInfo, nullptr, &pipelineLayout);
        if(result != VK_SUCCESS){
            throw std::runtime_error("failed to create compute pipeline layout!");
        }

        VkComputePipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
        pipelineInfo.stage = computeShaderStageInfo;
        pipelineInfo.layout = pipelineLayout;

        result = vkCreateComputePipelines(pCtx->device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline);
        if(result != VK_SUCCESS){
            throw std::runtime_error("failed to create compute pipeline!");
        }
        vkDestroyShaderModule(pCtx->device, computeShaderModule, nullptr);
    }
};


// class ComputeDescriptorSetLayoutBundle: public BaseDescriptorSetLayoutBundle{
// public:
//     void createDescriptorSetLayout() override {
//         // Implementation of compute descriptor set layout creation goes here
//         std::vector<VkDescriptorSetLayoutBinding> set0Bindings;

//         set0Bindings.push_back(createBinding(
//             0, 
//             VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 
//             VK_SHADER_STAGE_COMPUTE_BIT));
        
//         addLayout(set0Bindings);
//     }
// };
