#include <bitset>
#include <iostream>
#include "vulkan.h"
#include <vector>
#include <cstdlib>

class VulkanProgram
{
public:
    void run()
    {
        initVulkan();
    }

private:
    VkBool32 vkResult{};
    struct VulkanProgramInfo
    {
        VkInstance vulkanInstance = VK_NULL_HANDLE;
        VkPhysicalDevice GPU = VK_NULL_HANDLE;
        std::vector<const char *> LayerEnabled =
                {
                        "VK_LAYER_KHRONOS_validation"
                };

        std::vector<const char *> InstanceExtensionsEnabled =
                {
                };

    } vulkanProgramInfo;

    void initVulkan()
    {
        VkInstanceCreateInfo instanceCreateInfo{};
        instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        instanceCreateInfo.enabledExtensionCount = vulkanProgramInfo.InstanceExtensionsEnabled.size();
        instanceCreateInfo.ppEnabledExtensionNames = vulkanProgramInfo.InstanceExtensionsEnabled.data();
        instanceCreateInfo.enabledLayerCount = vulkanProgramInfo.LayerEnabled.size();
        instanceCreateInfo.ppEnabledLayerNames = vulkanProgramInfo.LayerEnabled.data();
        instanceCreateInfo.flags = 0;

        vkResult = vkCreateInstance(&instanceCreateInfo,
                                    nullptr,
                                    &vulkanProgramInfo.vulkanInstance);

        if (vkResult != VK_SUCCESS)
        {
            std::cout << "Failed to create Vulkan instance" << std::endl;
            exit(-1);
        }

        uint32_t physicalDeviceCount = 0;
        vkEnumeratePhysicalDevices(vulkanProgramInfo.vulkanInstance,
                                   &physicalDeviceCount,
                                   nullptr);

        std::vector<VkPhysicalDevice> GPUs(physicalDeviceCount);
        vkEnumeratePhysicalDevices(vulkanProgramInfo.vulkanInstance,
                                   &physicalDeviceCount,
                                   GPUs.data());

        vulkanProgramInfo.GPU = GPUs[0];
        VkPhysicalDeviceProperties gpuProperties{};

        vkGetPhysicalDeviceProperties(vulkanProgramInfo.GPU,
                                      &gpuProperties);

        std::cout << gpuProperties.deviceName << std::endl;
        VkFormatProperties formatProperties;
        vkGetPhysicalDeviceFormatProperties(vulkanProgramInfo.GPU,
                                            VK_FORMAT_R8G8B8A8_UNORM,
                                            &formatProperties);

        VkPhysicalDeviceMemoryProperties memoryProperties{};
        vkGetPhysicalDeviceMemoryProperties(vulkanProgramInfo.GPU, &memoryProperties);

        std::cout << "There are " << memoryProperties.memoryHeapCount << " heaps\n";
        for (int currHeapIndex = 0; currHeapIndex < memoryProperties.memoryHeapCount; currHeapIndex++)
        {
            // Acquire information about current heap
            std::cout << "  The heap at index " << currHeapIndex << " has the properties: \n";

            std::cout << "      Memory heap size: "
                      << (double) memoryProperties.memoryHeaps[currHeapIndex].size / (1024.0f * 1024.0f * 1024.0f)
                      << std::endl;

            std::cout << "      Memory heap property flags: "
                      << std::bitset<32>(memoryProperties.memoryHeaps[currHeapIndex].flags)
                      << std::endl;

            // Print out this heap's memory type information
            for (int currMemoryTypeIndex = 0;
                 currMemoryTypeIndex < memoryProperties.memoryTypeCount; currMemoryTypeIndex++)
            {
                if (memoryProperties.memoryTypes[currMemoryTypeIndex].heapIndex == currHeapIndex)
                {
                    std::cout << "          the memory type that is corresponding to this heap is " << std::endl;
                    std::cout << "          Memory type heap index: " << memoryProperties.memoryTypes[currMemoryTypeIndex].heapIndex
                              << std::endl;
                    std::cout << "          Memory type property flag: "
                              << std::bitset<32>(memoryProperties.memoryTypes[currMemoryTypeIndex].propertyFlags) << std::endl;
                }
            }
        }
		std::cout << "Maximum number of color attachment: " << gpuProperties.limits.maxColorAttachments << std::endl;
		std::cout << "Maximum framebuffer width: " << gpuProperties.limits.maxFramebufferWidth << std::endl;
		std::cout << "Maximum frambuffer height: " << gpuProperties.limits.maxFramebufferHeight << std::endl;
		std::cout << "Maximum frambuffer layer: " << gpuProperties.limits.maxFramebufferLayers << std::endl;
    }
};


int main()
{
    VulkanProgram program{};
    program.run();
}
