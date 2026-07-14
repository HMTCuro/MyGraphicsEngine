#pragma once

#include <vulkan/vulkan.h>

#include <fstream>
#include <vector>

class ShaderFactory {
public:
    static std::vector<char> ReadSpvFile(const std::string& filename);
    static VkShaderModule CreateShaderModule(VkDevice device, const std::vector<char>& code);
    static VkShaderModule CreateShaderModuleFromFile(VkDevice device, const std::string& filename);
    static VkPipelineShaderStageCreateInfo CreateShaderStageInfo(
        VkShaderStageFlagBits stage,
        VkShaderModule shaderModule,
        const char* entryPoint = "main",
        const VkSpecializationInfo* specializationInfo = nullptr
    );
    static VkRayTracingShaderGroupCreateInfoKHR CreateRTShaderGroup(
        VkRayTracingShaderGroupTypeKHR type,
        uint32_t generalShader,
        uint32_t closestHitShader,
        uint32_t anyHitShader,
        uint32_t intersectionShader
    );

};