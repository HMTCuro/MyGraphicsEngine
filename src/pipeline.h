#pragma once

#include <vulkan/vulkan.h>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE 
#define GLM_ENABLE_EXPERIMENTAL

#include "basicObjects.h"
#include "renderUtils.h"

#include <fstream>


const std::string vshPaths[] = {
    "../shader_spv/vert.spv",
    "../shader_spv/sampler_vert.spv"
};

const std::string fshPaths[] = {
    "../shader_spv/frag.spv",
    "../shader_spv/sampler_frag.spv"
};

class ShaderBindingTable{
public:
    enum SBTRegionType{
        eRAYGEN,
        eMISS,
        eHIT,
        eRegionCount
    };
    std::vector<VkBuffer> buffers;
    std::vector<VkDeviceMemory> bufferMemories;

    ShaderBindingTable(RendererContext* ctx, BufferManager* BufferManager){
        this->pCtx = ctx;
        this->pBufferManager = BufferManager;
    }

    void createSBTBuffers(){
        VkBufferUsageFlags usage = VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR |
        VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

        buffers.resize(SBTRegionType::eRegionCount);
        bufferMemories.resize(SBTRegionType::eRegionCount);

        // for(size_t i=0;i<SBTRegionType::eRegionCount;i++){
        //     pBufferManager->createBuffer(
        //         sbtRegionSizes[i], usage, 
        //         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
        //         buffers[i], bufferMemories[i]);
        // }
// allocator.createBuffer(sbtBuffer, bufferSize, usage, 
//                       VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE,
//                       VMA_ALLOCATION_CREATE_MAPPED_BIT,
//                       sbtGenerator.getBufferAlignment());
    }
private:
    RendererContext* pCtx;
    BufferManager* pBufferManager;
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
    VkRenderPass renderPass;
    VkPipelineLayout pipelineLayout;
    VkPipeline pipeline;
    std::vector<VkDescriptorSetLayout> descriptorSetLayouts;
    BufferManager* bufferManager;

    std::vector<char> readSpvFile(const std::string& filename) {
        std::ifstream file(filename, std::ios::ate | std::ios::binary);

        if (!file.is_open()) {
            throw std::runtime_error("failed to open file!");
        }

        size_t fileSize = (size_t) file.tellg();
        std::vector<char> buffer(fileSize);

        file.seekg(0);
        file.read(buffer.data(), fileSize);

        file.close();

        return buffer;
    }
    VkShaderModule createShaderModule(const std::vector<char>& code) {
        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = code.size();
        createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

        VkShaderModule shaderModule;
        VkResult result = vkCreateShaderModule(pCtx->device, &createInfo, nullptr, &shaderModule);
        if (result != VK_SUCCESS) {
            throw std::runtime_error("failed to create shader module!");
        }

        return shaderModule;
    }

};

class BaseDescriptorSetLayoutBundle{
public:
    std::vector<VkDescriptorSetLayout> getHandle(){
        return descriptorSetLayouts;
    }
    uint32_t getCount(){
        return static_cast<uint32_t>(descriptorSetLayouts.size());
    }
    void setProperties(VkDevice device){
        this->device = device;
    }
    virtual void createDescriptorSetLayout() = 0;
    void destroyDescriptorSetLayout() {
        for (const auto& descriptorSetLayout : descriptorSetLayouts) {
            vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
        }
    }
protected:
    VkDevice device;
    std::vector<VkDescriptorSetLayout> descriptorSetLayouts;
};

class WhiteModelPipeline1: public BasePipeline{
public:
    void createPipeline() override {
        std::vector<char> vertShaderCode = readSpvFile(vshPaths[0]);
        std::vector<char> fragShaderCode = readSpvFile(fshPaths[0]);

        VkShaderModule vertShaderModule= createShaderModule(vertShaderCode);
        VkShaderModule fragShaderModule= createShaderModule(fragShaderCode);

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

class WhiteModelDescriptorSetLayoutBundle: public BaseDescriptorSetLayoutBundle{
public:
    void createDescriptorSetLayout() override {
        VkDescriptorSetLayoutBinding uboLayoutBinding0{};
        uboLayoutBinding0.binding = 0;
        uboLayoutBinding0.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uboLayoutBinding0.descriptorCount = 1;
        uboLayoutBinding0.stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS;
        uboLayoutBinding0.pImmutableSamplers = nullptr;

        VkDescriptorSetLayoutBinding uboLayoutBinding1{};
        uboLayoutBinding1.binding = 1;
        uboLayoutBinding1.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uboLayoutBinding1.descriptorCount = 1;
        uboLayoutBinding1.stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS;
        uboLayoutBinding1.pImmutableSamplers = nullptr;

        std::array<VkDescriptorSetLayoutBinding,2> bindings = {uboLayoutBinding0, uboLayoutBinding1};
        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
        layoutInfo.pBindings = bindings.data();

        VkDescriptorSetLayout descriptorSetLayout;

        VkResult result = vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout);
        if(result != VK_SUCCESS){
            throw std::runtime_error("failed to create descriptor set layout!");
        }

        descriptorSetLayouts.push_back(descriptorSetLayout);
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
        const char* rayGenShaderPath = "../shader_spv/raygen.spv";
        const char* missShaderPath = "../shader_spv/miss.spv";
        const char* hitShaderPath = "../shader_spv/closeHit.spv";
        std::vector<char> rayGenShaderCode = readSpvFile(rayGenShaderPath);
        std::vector<char> missShaderCode = readSpvFile(missShaderPath);
        std::vector<char> hitShaderCode = readSpvFile(hitShaderPath);
        VkShaderModule rayGenShaderModule = createShaderModule(rayGenShaderCode);
        VkShaderModule missShaderModule = createShaderModule(missShaderCode);
        VkShaderModule hitShaderModule = createShaderModule(hitShaderCode);

        std::cout << "Shader modules created successfully." << std::endl;

        enum StageIndices {
            eRaygen,
            eMiss,
            eClosestHit,
            eShaderGroupCount
        };

        VkPipelineShaderStageCreateInfo rayGenShaderStageInfo{};
        rayGenShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        rayGenShaderStageInfo.stage = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
        rayGenShaderStageInfo.module = rayGenShaderModule;
        rayGenShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo missShaderStageInfo{};
        missShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        missShaderStageInfo.stage = VK_SHADER_STAGE_MISS_BIT_KHR;
        missShaderStageInfo.module = missShaderModule;
        missShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo hitShaderStageInfo{};
        hitShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        hitShaderStageInfo.stage = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
        hitShaderStageInfo.module = hitShaderModule;
        hitShaderStageInfo.pName = "main";

        std::array<VkPipelineShaderStageCreateInfo, eShaderGroupCount> shaderStages = { rayGenShaderStageInfo, missShaderStageInfo, hitShaderStageInfo };

        VkRayTracingShaderGroupCreateInfoKHR group{};
        group.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
        group.anyHitShader = VK_SHADER_UNUSED_KHR;
        group.closestHitShader = VK_SHADER_UNUSED_KHR;
        group.generalShader = VK_SHADER_UNUSED_KHR;
        group.intersectionShader = VK_SHADER_UNUSED_KHR;

        std::vector<VkRayTracingShaderGroupCreateInfoKHR> shaderGroups;

        group.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
        group.generalShader = eRaygen;
        shaderGroups.push_back(group);

        group.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
        group.generalShader = eMiss;
        shaderGroups.push_back(group);

        group.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
        group.closestHitShader = eClosestHit;
        group.generalShader = VK_SHADER_UNUSED_KHR;
        shaderGroups.push_back(group);

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
        rtPipelineInfo.maxPipelineRayRecursionDepth = 1;

        auto fpCreateRayTracingPipelinesKHR = (PFN_vkCreateRayTracingPipelinesKHR)vkGetDeviceProcAddr(pCtx->device, "vkCreateRayTracingPipelinesKHR");
        result = fpCreateRayTracingPipelinesKHR(pCtx->device, VK_NULL_HANDLE, VK_NULL_HANDLE, 1, &rtPipelineInfo, nullptr, &pipeline);
        if(result != VK_SUCCESS){
            throw std::runtime_error("failed to create ray tracing pipeline!");
        }

        vkDestroyShaderModule(pCtx->device, rayGenShaderModule, nullptr);
        vkDestroyShaderModule(pCtx->device, missShaderModule, nullptr);
        vkDestroyShaderModule(pCtx->device, hitShaderModule, nullptr);

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


        uint32_t raygenSize   = alignUp(handleSize, handleAlignment);
        uint32_t missSize     = alignUp(handleSize, handleAlignment);
        uint32_t hitSize      = alignUp(handleSize, handleAlignment);
        uint32_t callableSize = 0;  // No callable shaders in this tutorial

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


        // Hit shader (group 2)
        memcpy(pData + hitOffset, shaderHandles.data() + 2 * handleSize, handleSize);
        hitRegion.deviceAddress = bufferManager->getBufferDeviceAddress(sbtBuffer) + hitOffset;
        hitRegion.stride        = hitSize;
        hitRegion.size          = hitSize;

        // Callable shaders (none in this tutorial)
        callableRegion.deviceAddress = 0;
        callableRegion.stride        = 0;
        callableRegion.size          = 0;

    // LOGI("Shader binding table created and populated \n");
        
    }
};

class RayTracingDescriptorSetLayoutBundle: public BaseDescriptorSetLayoutBundle{
public:
    void createDescriptorSetLayout() override {
        // Implementation of ray tracing descriptor set layout creation goes here
        // set 0 binding 0: tlas
        // set 0 binding 1: output image
        // set 0 binding 2: instance info buffer
        // set 1 binding 0: global uniform buffer (window size, clear color, scale)
        // set 1 binding 1: camera uniform buffer
        // set 1 binding 2: light uniform buffer
        VkResult result;

        VkDescriptorSetLayoutBinding tlasBinding{};
        tlasBinding.binding = 0;
        tlasBinding.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
        tlasBinding.descriptorCount = 1;
        tlasBinding.stageFlags = VK_SHADER_STAGE_ALL;

        VkDescriptorSetLayoutBinding outImageBinding{};
        outImageBinding.binding = 1;
        outImageBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        outImageBinding.descriptorCount = 1;
        outImageBinding.stageFlags = VK_SHADER_STAGE_ALL;

        VkDescriptorSetLayoutBinding instanceInfoBinding{};
        instanceInfoBinding.binding = 2;
        instanceInfoBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        instanceInfoBinding.descriptorCount = 1;
        instanceInfoBinding.stageFlags = VK_SHADER_STAGE_ALL;

        VkDescriptorSetLayoutBinding globalUniformBinding{};
        globalUniformBinding.binding = 0;
        globalUniformBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        globalUniformBinding.descriptorCount = 1;
        globalUniformBinding.stageFlags = VK_SHADER_STAGE_ALL;

        VkDescriptorSetLayoutBinding cameraUniformBinding{};
        cameraUniformBinding.binding = 1;
        cameraUniformBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        cameraUniformBinding.descriptorCount = 1;
        cameraUniformBinding.stageFlags = VK_SHADER_STAGE_ALL;

        VkDescriptorSetLayoutBinding lightUniformBinding{};
        lightUniformBinding.binding = 2;
        lightUniformBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        lightUniformBinding.descriptorCount = 1;
        lightUniformBinding.stageFlags = VK_SHADER_STAGE_ALL;

        std::array<VkDescriptorSetLayoutBinding, 3> set0Bindings = {tlasBinding, outImageBinding, instanceInfoBinding};
        std::array<VkDescriptorSetLayoutBinding, 3> set1Bindings = {globalUniformBinding, cameraUniformBinding, lightUniformBinding};

        VkDescriptorSetLayoutCreateInfo layoutInfo0{};
        layoutInfo0.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo0.bindingCount = static_cast<uint32_t>(set0Bindings.size());
        layoutInfo0.pBindings = set0Bindings.data();

        VkDescriptorSetLayoutCreateInfo layoutInfo1{};
        layoutInfo1.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo1.bindingCount = static_cast<uint32_t>(set1Bindings.size());
        layoutInfo1.pBindings = set1Bindings.data();

        VkDescriptorSetLayout descriptorSetLayout0;
        VkDescriptorSetLayout descriptorSetLayout1;

        result = vkCreateDescriptorSetLayout(device, &layoutInfo0, nullptr, &descriptorSetLayout0);
        if(result != VK_SUCCESS){
            throw std::runtime_error("failed to create ray tracing descriptor set layout 0!");
        }

        result = vkCreateDescriptorSetLayout(device, &layoutInfo1, nullptr, &descriptorSetLayout1);
        if(result != VK_SUCCESS){
            throw std::runtime_error("failed to create ray tracing descriptor set layout 1!");
        }

        descriptorSetLayouts.push_back(descriptorSetLayout0);
        descriptorSetLayouts.push_back(descriptorSetLayout1);

        /***
         * rgen:
        layout (binding = 0, set = 0) uniform accelerationStructureEXT tlas;
        layout (binding = 1, set = 0, rgba32f) uniform image2D image;
        layout (binding = 0, set = 1) uniform G {
            vec2 window_size;
            vec3 clear_color;
            int scale;
        } g;
        layout(binding = 1, set = 1) uniform CameraUniform {
            vec3 pos;
            mat4 view;
            mat4 project;
        } camera;
        layout(location = 0) rayPayloadEXT RayPayload prd;
        
        *rchit:
        layout(binding = 0, set = 0) uniform accelerationStructureEXT tlas;
        layout(location = 0) rayPayloadInEXT RayPayload payload;
        layout(location = 1) rayPayloadEXT bool isShadow;
        layout(binding = 2,set=0,scalar) buffer InstanceInfo_ {
        InstanceInfo infos[];
        } info_;
        layout(binding=3,set=1) uniform Light_{
        vec3 pos;
        vec3 color;
        float intensity;
        } light;

        *rmiss:
        layout(location = 1) rayPayloadInEXT bool isShadow;
        ***/
    }
};

class TextureSamplerPipeline: public BasePipeline{
public:
    void createPipeline() override {
        // 1. 读取全屏顶点和片元着色器
        std::vector<char> vertShaderCode = readSpvFile(vshPaths[1]);
        std::vector<char> fragShaderCode = readSpvFile(fshPaths[1]);
        VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
        VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

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

class TextureSamplerDescriptorSetLayoutBundle: public BaseDescriptorSetLayoutBundle{
public:
    void createDescriptorSetLayout() override {
        VkDescriptorSetLayoutBinding textureBinding{};
        textureBinding.binding = 0;
        textureBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        textureBinding.descriptorCount = 1;
        textureBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        std::array<VkDescriptorSetLayoutBinding, 1> bindings = {textureBinding};

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
        layoutInfo.pBindings = bindings.data();

        VkDescriptorSetLayout descriptorSetLayout;

        VkResult result = vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout);
        if(result != VK_SUCCESS){
            throw std::runtime_error("failed to create texture sampler descriptor set layout!");
        }

        descriptorSetLayouts.push_back(descriptorSetLayout);
    }
};