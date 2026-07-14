#include "shaderUtils.h"

std::vector<char> ShaderFactory::ReadSpvFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);
    if (!file.is_open()) throw std::runtime_error("failed to open file!");
    size_t fileSize = (size_t)file.tellg();
    std::vector<char> buffer(fileSize);
    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();
    return buffer;
}

VkShaderModule ShaderFactory::CreateShaderModule(VkDevice device, const std::vector<char>& code) {
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
    VkShaderModule shaderModule;
    if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
        throw std::runtime_error("failed to create shader module!");
    return shaderModule;
}

VkShaderModule ShaderFactory::CreateShaderModuleFromFile(VkDevice device, const std::string& filename) {
    auto code = ReadSpvFile(filename);
    return CreateShaderModule(device, code);
}

VkPipelineShaderStageCreateInfo ShaderFactory::CreateShaderStageInfo(
    VkShaderStageFlagBits stage,
    VkShaderModule shaderModule,
    const char* entryPoint,
    const VkSpecializationInfo* specializationInfo)
{
    VkPipelineShaderStageCreateInfo shaderStageInfo{};
    shaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStageInfo.stage = stage;
    shaderStageInfo.module = shaderModule;
    shaderStageInfo.pName = entryPoint;
    shaderStageInfo.pSpecializationInfo = specializationInfo;
    return shaderStageInfo;
}

VkRayTracingShaderGroupCreateInfoKHR ShaderFactory::CreateRTShaderGroup(
    VkRayTracingShaderGroupTypeKHR type,
    uint32_t generalShader,
    uint32_t closestHitShader,
    uint32_t anyHitShader,
    uint32_t intersectionShader
) {
    VkRayTracingShaderGroupCreateInfoKHR group{};
    group.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
    group.type = type;
    group.generalShader = generalShader;
    group.closestHitShader = closestHitShader;
    group.anyHitShader = anyHitShader;
    group.intersectionShader = intersectionShader;
    return group;
}


