//
// Created by Jiaxiong Jiao on 2022-04-17.
//

#ifndef VULKANPROGRAM_VULKAN_HELPERS_H
#define VULKANPROGRAM_VULKAN_HELPERS_H

#include <vulkan/vulkan.h>
#include <iostream>

void createBuffer(VkBufferCreateInfo &bufferCreateInfo,
                  VkDevice &targetDevice,
                  VkBuffer &buffer,
                  VkPhysicalDevice &physicalDevice,
                  VkDeviceMemory &bufferMemory,
                  VkMemoryPropertyFlags memoryPropertyFlags)
{
    VkResult result = vkCreateBuffer(targetDevice,
                                     &bufferCreateInfo,
                                     nullptr, &buffer);

    if (result != VK_SUCCESS)
    {
        std::cerr << "Failed to create buffer" << std::endl;
        exit(1);
    }

    VkMemoryRequirements bufferMemoryRequirements;
    vkGetBufferMemoryRequirements(targetDevice,
                                  buffer,
                                  &bufferMemoryRequirements);

    VkPhysicalDeviceMemoryProperties physicalDeviceMemoryProperties;

    vkGetPhysicalDeviceMemoryProperties(physicalDevice,
                                        &physicalDeviceMemoryProperties);

    VkMemoryAllocateInfo memoryAllocateInfo{};
    memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memoryAllocateInfo.pNext = nullptr;
    memoryAllocateInfo.allocationSize = bufferMemoryRequirements.size;

    bool foundSuitableMemoryType = false;
    for (std::size_t i = 0; i < physicalDeviceMemoryProperties.memoryTypeCount; i++)
    {
        if (bufferMemoryRequirements.memoryTypeBits & (1 << i) &&
            (physicalDeviceMemoryProperties.memoryTypes[i].propertyFlags & memoryPropertyFlags) == memoryPropertyFlags)
        {
            memoryAllocateInfo.memoryTypeIndex = i;
            foundSuitableMemoryType = true;
            break;
        }
    }

    if (!foundSuitableMemoryType)
    {
        std::cerr << "Failed to find suitable memory type" << std::endl;
        exit(1);
    }

    vkAllocateMemory(targetDevice,
                     &memoryAllocateInfo,
                     nullptr,
                     &bufferMemory);

    vkBindBufferMemory(targetDevice,
                       buffer,
                       bufferMemory,
                       0);
}

#endif //VULKANPROGRAM_VULKAN_HELPERS_H
