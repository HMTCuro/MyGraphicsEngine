#pragma once

#include <vulkan/vulkan.h>

#include <fstream>
#include <vector>

class ShaderFactory {
public:
    static std::vector<char> ReadSpvFile(const std::string& filename);
    static VkShaderModule CreateShaderModule(VkDevice device, const std::vector<char>& code);
    static VkShaderModule CreateShaderModuleFromFile(VkDevice device, const std::string& filename);
};