#include <bitset>
#include <iostream>
#include "vulkan.h"
#include <vector>
#include <cstdlib>

class VulkanProgram {
public:
    void run() {
        initVulkan();
    }

private:
    VkBool32 vkResult{};
    struct VulkanProgramInfo {
        VkInstance vulkanInstance = VK_NULL_HANDLE;
        VkPhysicalDevice GPU;
        std::vector<const char *> LayerEnabled =
                {
                    "VK_LAYER_KHRONOS_validation"
                };

        std::vector<const char *> InstanceExtensionsEnabled =
                {
                };

    } vulkanProgramInfo;

    void initVulkan() {
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

        if (vkResult != VK_SUCCESS) {
            std::cout << "Failed to create Vulkan instance" << std::endl;
            exit(-1);
        }

        uint32_t  physicalDeviceCount = 0;
        vkEnumeratePhysicalDevices(vulkanProgramInfo.vulkanInstance,
                                   &physicalDeviceCount,
                                   nullptr);

        std::vector<VkPhysicalDevice> GPUs(physicalDeviceCount);
        vkEnumeratePhysicalDevices(vulkanProgramInfo.vulkanInstance,
                                   &physicalDeviceCount,
                                   GPUs.data());

        vulkanProgramInfo.GPU = GPUs[0];
        VkFormatProperties formatProperties;
        vkGetPhysicalDeviceFormatProperties(vulkanProgramInfo.GPU,
                                            VK_FORMAT_R8G8B8A8_UNORM,
                                            &formatProperties);

        VkPhysicalDeviceMemoryProperties memoryProperties{};
        vkGetPhysicalDeviceMemoryProperties(vulkanProgramInfo.GPU, &memoryProperties);
        std::cout << "Memory type count: " << memoryProperties.memoryTypeCount << std::endl;
        std::cout << "Memory type heap index: " << memoryProperties.memoryTypes->heapIndex << std::endl;
        std::cout << "Memory type property flag: " << std::bitset<32>(memoryProperties.memoryTypes->propertyFlags) << std::endl;

        std::cout << "Memory heap count: " << memoryProperties.memoryHeapCount << std::endl;
        std::cout << "Memory heap size: " << memoryProperties.memoryHeaps->size << std::endl;
        std::cout << "Memory heap property flags: " << std::bitset<32>(memoryProperties.memoryHeaps->flags) << std::endl;
    }
};


int main() {
    VulkanProgram program{};
    program.run();
}
