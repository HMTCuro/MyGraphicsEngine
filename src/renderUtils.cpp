#include "renderUtils.h"

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

