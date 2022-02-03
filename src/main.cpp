#include <bitset>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <cstdlib>

const int windowWidth = 800;
const int windowHeight = 600;

class VulkanProgram
{
public:
    void run()
    {
        glfwInit();
        addAdditionalInstanceExtensions();
        initVulkan();
        createDebugMessenger();
        pickPhysicalDevice();
        addAdditionalDeviceExtensions();
        createDeviceAndGraphicsQueue();

        // Graphics Setup
        createWindowAndSurface();

        // Program Loop
        programLoop();
        cleanup();
    }

private:
    VkResult vkResult{};

    VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;

    struct VulkanProgramInfo
    {
        GLFWwindow *window;
        VkInstance vulkanInstance = VK_NULL_HANDLE;

        VkPhysicalDevice GPU = VK_NULL_HANDLE;

        VkDevice renderDevice = VK_NULL_HANDLE;

        VkQueue graphicsQueue = VK_NULL_HANDLE;

        VkSurfaceKHR windowSurface = VK_NULL_HANDLE;

        uint32_t graphicsQueueFamilyIndex = 0;
        bool graphicsQueueFound = false;

        std::vector<const char *> layerEnabled =
                {
                        "VK_LAYER_KHRONOS_validation",
                };

        std::vector<const char *> instanceExtensionsEnabled =
                {
                        VK_EXT_DEBUG_UTILS_EXTENSION_NAME
                };

        std::vector<const char *> deviceExtensionsEnabled =
                {

                };

    } vulkanProgramInfo;

    void initVulkan()
    {

        VkInstanceCreateInfo instanceCreateInfo{};
        instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        instanceCreateInfo.enabledExtensionCount = vulkanProgramInfo.instanceExtensionsEnabled.size();
        instanceCreateInfo.ppEnabledExtensionNames = vulkanProgramInfo.instanceExtensionsEnabled.data();
        instanceCreateInfo.enabledLayerCount = vulkanProgramInfo.layerEnabled.size();
        instanceCreateInfo.ppEnabledLayerNames = vulkanProgramInfo.layerEnabled.data();
        instanceCreateInfo.flags = 0;

        // Fill in a debug messenger create info structure to put in instance create info
        VkDebugUtilsMessengerCreateInfoEXT debugMessengerCreateInfo{};
        fillInDebugMessengerCreateInfo(debugMessengerCreateInfo);

        // After it is filled, put it in pNext of instance create info.
        instanceCreateInfo.pNext = &debugMessengerCreateInfo;

        vkResult = vkCreateInstance(&instanceCreateInfo,
                                    nullptr,
                                    &vulkanProgramInfo.vulkanInstance);

        if (vkResult != VK_SUCCESS)
        {
            std::cout << "Failed to create Vulkan instance" << std::endl;
            exit(-1);
        }
    }

    void createDebugMessenger()
    {
        // Find the function for creating debug messenger
        auto createDebugUtilsMessenger = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(
                vulkanProgramInfo.vulkanInstance,
                "vkCreateDebugUtilsMessengerEXT");

        VkDebugUtilsMessengerCreateInfoEXT debugMessengerCreateInfo{};
        fillInDebugMessengerCreateInfo(debugMessengerCreateInfo);

        createDebugUtilsMessenger(vulkanProgramInfo.vulkanInstance,
                                  &debugMessengerCreateInfo,
                                  nullptr,
                                  &debugMessenger);
    }

    void pickPhysicalDevice()
    {
        uint32_t physicalDeviceCount;
        vkEnumeratePhysicalDevices(vulkanProgramInfo.vulkanInstance,
                                   &physicalDeviceCount,
                                   nullptr);
        std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);

        vkEnumeratePhysicalDevices(vulkanProgramInfo.vulkanInstance,
                                   &physicalDeviceCount,
                                   physicalDevices.data());

        vulkanProgramInfo.GPU = physicalDevices[0];
    }

    void createDeviceAndGraphicsQueue()
    {
        uint32_t queueFamilyPptCount;
        vkGetPhysicalDeviceQueueFamilyProperties(vulkanProgramInfo.GPU,
                                                 &queueFamilyPptCount,
                                                 nullptr);

        std::vector<VkQueueFamilyProperties> queueFamilyPptList(queueFamilyPptCount);

        vkGetPhysicalDeviceQueueFamilyProperties(vulkanProgramInfo.GPU,
                                                 &queueFamilyPptCount,
                                                 queueFamilyPptList.data());

        for (std::size_t i = 0; i < queueFamilyPptCount; i++)
        {
            if (VK_QUEUE_GRAPHICS_BIT & queueFamilyPptList[i].queueFlags)
            {
                vulkanProgramInfo.graphicsQueueFamilyIndex = i;
                vulkanProgramInfo.graphicsQueueFound = true;
                break;
            }
        }

        if (!vulkanProgramInfo.graphicsQueueFound)
        {
            std::cout << "Graphics queue not found!\n";
            exit(-1);
        }

        // Logical Device Creation
        // Fill queue creation info first
        float graphicsQueuePriority = 1.0f;
        VkDeviceQueueCreateInfo graphicsQueueCreateInfo{};
        graphicsQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        graphicsQueueCreateInfo.queueCount = 1;
        graphicsQueueCreateInfo.pNext = nullptr;
        graphicsQueueCreateInfo.flags = 0;
        graphicsQueueCreateInfo.pQueuePriorities = &graphicsQueuePriority;
        graphicsQueueCreateInfo.queueFamilyIndex = vulkanProgramInfo.graphicsQueueFamilyIndex;

        // Logical device creat info
        VkDeviceCreateInfo renderDeviceCreateInfo{};
        renderDeviceCreateInfo.flags = 0;
        renderDeviceCreateInfo.pNext = nullptr;
        renderDeviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        renderDeviceCreateInfo.enabledExtensionCount = vulkanProgramInfo.deviceExtensionsEnabled.size();
        renderDeviceCreateInfo.ppEnabledExtensionNames = vulkanProgramInfo.deviceExtensionsEnabled.data();
        renderDeviceCreateInfo.enabledLayerCount = 0;
        renderDeviceCreateInfo.pEnabledFeatures = nullptr;
        renderDeviceCreateInfo.pQueueCreateInfos = &graphicsQueueCreateInfo;
        renderDeviceCreateInfo.queueCreateInfoCount = 1;

        vkResult = vkCreateDevice(vulkanProgramInfo.GPU,
                                  &renderDeviceCreateInfo,
                                  nullptr,
                                  &vulkanProgramInfo.renderDevice);

        checkVkResult(vkResult, "");

        vkGetDeviceQueue(vulkanProgramInfo.renderDevice,
                         vulkanProgramInfo.graphicsQueueFamilyIndex,
                         0,
                         &vulkanProgramInfo.graphicsQueue);
    }

    void createWindowAndSurface()
    {
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        vulkanProgramInfo.window = glfwCreateWindow(windowWidth,
                                                    windowHeight,
                                                    "Vulkan Program",
                                                    nullptr,
                                                    nullptr);

        vkResult = glfwCreateWindowSurface(vulkanProgramInfo.vulkanInstance,
                                           vulkanProgramInfo.window,
                                           nullptr,
                                           &vulkanProgramInfo.windowSurface);
    }

    void programLoop()
    {
        while(!glfwWindowShouldClose(vulkanProgramInfo.window))
        {
            glfwPollEvents();
        }
    }

    void cleanup() const
    {
        vkDestroyDevice(vulkanProgramInfo.renderDevice,
                        nullptr);

        // Find function to destroy debug messenger
        auto destroyDebugMessenger = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(
                vulkanProgramInfo.vulkanInstance,
                "vkDestroyDebugUtilsMessengerEXT"
        );
        destroyDebugMessenger(vulkanProgramInfo.vulkanInstance,
                              debugMessenger,
                              nullptr);
        vkDestroySurfaceKHR(vulkanProgramInfo.vulkanInstance,
                            vulkanProgramInfo.windowSurface,
                            nullptr);

        vkDestroyInstance(vulkanProgramInfo.vulkanInstance, nullptr);

        glfwDestroyWindow(vulkanProgramInfo.window);

    }

    void addAdditionalInstanceExtensions()
    {
        uint32_t extensionPptCount;
        vkEnumerateInstanceExtensionProperties(nullptr,
                                               &extensionPptCount,
                                               nullptr);

        std::vector<VkExtensionProperties> extensionPptList(extensionPptCount);

        vkEnumerateInstanceExtensionProperties(nullptr,
                                               &extensionPptCount,
                                               extensionPptList.data());

        // If VK_KHR_get_physical_device_properties2 is available add it in.
        for (std::size_t i = 0; i < extensionPptCount; i++)
        {
            if (strcmp(extensionPptList[i].extensionName, "VK_KHR_get_physical_device_properties2") == 0)
            {
                vulkanProgramInfo.instanceExtensionsEnabled.push_back("VK_KHR_get_physical_device_properties2");
            }
        }

        // Add extensions required by GLFW
        uint32_t glfwRequiredExtensionCount;
        const char** glfwInstanceExtensions;
        glfwInstanceExtensions = glfwGetRequiredInstanceExtensions(&glfwRequiredExtensionCount);

        if (glfwInstanceExtensions == nullptr)
        {
            std::cout << "Failed to get instance extensions required by glfw" << std::endl;
            exit(-1);
        }

        for (std::size_t i = 0; i < glfwRequiredExtensionCount;i++)
        {
            vulkanProgramInfo.instanceExtensionsEnabled.push_back(glfwInstanceExtensions[i]);
        }

    }

    void addAdditionalDeviceExtensions()
    {
        uint32_t extensionPptCount;
        vkEnumerateDeviceExtensionProperties(vulkanProgramInfo.GPU,
                                             nullptr,
                                             &extensionPptCount,
                                             nullptr);

        std::vector<VkExtensionProperties> extensionPptList(extensionPptCount);

        vkEnumerateDeviceExtensionProperties(vulkanProgramInfo.GPU,
                                             nullptr,
                                             &extensionPptCount,
                                             extensionPptList.data());

        // If there is a portability subset device extension, add it in.
        for (std::size_t i = 0; i < extensionPptCount; i++)
        {
            if (strcmp(extensionPptList[i].extensionName, "VK_KHR_portability_subset") == 0)
            {
                vulkanProgramInfo.deviceExtensionsEnabled.push_back("VK_KHR_portability_subset");
                break;
            }
        }
    }

    static void checkVkResult(const VkResult &result, const char *failMessage)
    {
        if (result == VK_SUCCESS) return;
        std::cout << failMessage << std::endl;
        exit(-1);
    }

    static void fillInDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &createInfo)
    {
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo.flags = 0;
        createInfo.pNext = nullptr;
        createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
                                     VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;

        createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT;

        createInfo.pfnUserCallback = debugCallback;
    }

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
            VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
            VkDebugUtilsMessageTypeFlagsEXT messageType,
            const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
            [[maybe_unused]] void *pUserData
    )
    {
        std::cout << "This is your customized debug callback for VkDebugUtilsMessenger\n";

        if (messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)
        {
            std::cout << "Message Severity: VERBOSE";
        } else if (messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
        {
            std::cout << "Message Severity: WARNING";
        } else if (messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
        {
            std::cout << "Message Severity: ERROR";
        } else if (messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
        {
            std::cout << "Message Severity: INFO";
        } else
        {
            std::cout << "Message Severity: UNKNOWN";
        }

        std::cout << "\n";

        if (messageType == VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT)
        {
            std::cout << "This is a GENERAL message";
        } else if (messageType == VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT)
        {
            std::cout << "This is a VALIDATION message";
        } else if (messageType == VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT)
        {
            std::cout << "This is a PERFORMANCE message";
        } else
        {
            std::cout << "This is an UNKNOWN message";
        }

        std::cout << "\n";
        std::cout << "Message ID Name: " << pCallbackData->pMessageIdName << std::endl;
        std::cout << "Message ID Number: " << pCallbackData->messageIdNumber << std::endl;
        std::cout << "Message is: " << pCallbackData->pMessage << std::endl;
        std::cout << "== Finished ==\n\n\n" << std::endl;

        return VK_FALSE;
    }
};


int main()
{
    VulkanProgram program{};
    program.run();
}
