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
                    "VK_LAYER_KHRONOS_validation",
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

		// Choose which GPU to use
        vulkanProgramInfo.GPU = GPUs[0];
    }
};


int main()
{
    VulkanProgram program{};
    program.run();
}
