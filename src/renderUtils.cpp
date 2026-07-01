#include "renderUtils.h"

// ----------------------------RenderUtils Implementation----------------------------
uint32_t RenderUtils::findMemoryType(uint32_t typeFilter,VkMemoryPropertyFlags properties, VkPhysicalDevice physicalDevice){
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    throw std::runtime_error("failed to find suitable memory type!");
}

void RenderUtils::createCommandBuffers(RendererContext* pCtx, std::vector<VkCommandBuffer>& commandBuffers, uint32_t commandBufferCount){
    commandBuffers.resize(commandBufferCount);

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = pCtx->commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = commandBufferCount;

    if (vkAllocateCommandBuffers(pCtx->device, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate command buffers!");
    }
}

VkDescriptorSetLayoutBinding RenderUtils::createBinding(uint32_t binding, VkDescriptorType type, VkShaderStageFlags stageFlags, uint32_t descriptorCount){
    VkDescriptorSetLayoutBinding layoutBinding{};
    layoutBinding.binding = binding;
    layoutBinding.descriptorType = type;
    layoutBinding.descriptorCount = descriptorCount;
    layoutBinding.stageFlags = stageFlags;
    return layoutBinding;
}

std::vector<VkDescriptorPoolSize> RenderUtils::combineDescriptorPoolSizes(const std::vector<std::vector<VkDescriptorPoolSize>>& sizes) {
    std::unordered_map<VkDescriptorType, uint32_t> combinedMap;

    for (const auto& sizeList : sizes) {
        for (const auto& size : sizeList) {
            combinedMap[size.type] += size.descriptorCount;
        }
    }

    std::vector<VkDescriptorPoolSize> combinedSizes;
    for (const auto& pair : combinedMap) {
        VkDescriptorPoolSize poolSize{};
        poolSize.type = pair.first;
        poolSize.descriptorCount = pair.second;
        combinedSizes.push_back(poolSize);
    }

    return combinedSizes;
}

// ----------------------------ImageFactory Implementation----------------------------
void ImageFactory::createImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory, VkPhysicalDevice physicalDevice, VkDevice device) {
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = mipLevels;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage;
    imageInfo.samples = numSamples;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkResult result = vkCreateImage(device, &imageInfo, nullptr, &image);
    if (result != VK_SUCCESS) {
        throw std::runtime_error("failed to create image!");
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device, image, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = RenderUtils::findMemoryType(memRequirements.memoryTypeBits, properties, physicalDevice);

    result = vkAllocateMemory(device, &allocInfo, nullptr, &imageMemory);
    if (result != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate image memory!");
    }

    vkBindImageMemory(device, image, imageMemory, 0);
}

VkImageView ImageFactory::createImageView(VkImage image, VkFormat format,VkImageAspectFlags aspectFlags, uint32_t miplevels, VkDevice device) {
    VkImageViewCreateInfo createInfo{};
    createInfo.sType=VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    createInfo.image=image;
    createInfo.viewType=VK_IMAGE_VIEW_TYPE_2D;
    createInfo.format=format;
    createInfo.subresourceRange.aspectMask=aspectFlags;
    createInfo.subresourceRange.baseMipLevel=0;
    createInfo.subresourceRange.levelCount=miplevels;
    createInfo.subresourceRange.baseArrayLayer=0;
    createInfo.subresourceRange.layerCount=1;

    VkImageView imageView;
    VkResult result = vkCreateImageView(device, &createInfo, nullptr, &imageView);
    if(result != VK_SUCCESS){
        throw std::runtime_error("failed to create image view!");
    }
    return imageView;
}

void DescriptorWriteCollector::addBuffer(VkDescriptorSet dstSet, uint32_t binding, VkDescriptorType type, VkBuffer buffer, VkDeviceSize offset, VkDeviceSize range, uint32_t arrayElement) {
    VkDescriptorBufferInfo info{};
    info.buffer = buffer;
    info.offset = offset;
    info.range  = range;
    bufferInfos.push_back(info);

    VkWriteDescriptorSet write{};
    write.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet          = dstSet;
    write.dstBinding      = binding;
    write.dstArrayElement = arrayElement;
    write.descriptorCount = 1;
    write.descriptorType  = type;
    write.pBufferInfo     = &bufferInfos.back();
    writes.push_back(write);
}

void DescriptorWriteCollector::addImage(VkDescriptorSet dstSet,
                uint32_t binding,
                VkDescriptorType type,         // VK_DESCRIPTOR_TYPE_STORAGE_IMAGE / COMBINED_IMAGE_SAMPLER
                VkImageView imageView,
                VkImageLayout imageLayout,
                VkSampler sampler,
                uint32_t arrayElement)
{
    VkDescriptorImageInfo info{};
    info.sampler     = sampler;
    info.imageView   = imageView;
    info.imageLayout = imageLayout;
    imageInfos.push_back(info);

    VkWriteDescriptorSet write{};
    write.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet          = dstSet;
    write.dstBinding      = binding;
    write.dstArrayElement = arrayElement;
    write.descriptorCount = 1;
    write.descriptorType  = type;
    write.pImageInfo      = &imageInfos.back();
    writes.push_back(write);
}

void DescriptorWriteCollector::addAccelerationStructures(VkDescriptorSet dstSet,
                                   uint32_t binding,
                                   const std::vector<VkAccelerationStructureKHR>& handles,
                                   uint32_t arrayElement)
{
    // 保存 handles 的副本，并记录此次写入的偏移
    size_t offset = asHandles.size();
    asHandles.insert(asHandles.end(), handles.begin(), handles.end());

    VkWriteDescriptorSetAccelerationStructureKHR asExt{};
    asExt.sType                      = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
    asExt.accelerationStructureCount = static_cast<uint32_t>(handles.size());
    asExt.pAccelerationStructures    = &asHandles[offset];   // 指向稳定存储
    asExtensions.push_back(asExt);

    VkWriteDescriptorSet write{};
    write.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.pNext           = &asExtensions.back();
    write.dstSet          = dstSet;
    write.dstBinding      = binding;
    write.dstArrayElement = arrayElement;
    write.descriptorCount = 1;
    write.descriptorType  = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
    writes.push_back(write);
}