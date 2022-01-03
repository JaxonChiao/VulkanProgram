#include <cmath>
#include <iostream>
#include "vulkan.h"
#include <vector>

class VulkanProgram
{
    public:
        void run()
            {
                initVulkan();
            }
    private:
        void initVulkan()
        {
            uint32_t layerCount = 0;
            vkEnumerateInstanceLayerProperties(&layerCount,
                                                nullptr);
            std::cout << layerCount << std::endl;

            std::vector<VkLayerProperties> layers;
            layers.resize(layerCount);

            vkEnumerateInstanceLayerProperties(&layerCount, layers.data());

            for (const auto& layer : layers)
            {
                std::cout << layer.layerName << std::endl;
            }
        }
    };


int main()
{
    VulkanProgram program{};
    program.run();
}
