#include <bitset>

#define GLFW_INCLUDE_VULKAN

#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <cstdlib>
#include <fstream>
#include <cstring>
#include <algorithm>

const uint32_t windowWidth = 800;
const uint32_t windowHeight = 600;

class VulkanProgram
{
public:
    void run()
    {
        // Create a window for presentation
        glfwInit();

        // Allow window system to add its own required
        // instance extensions
        addAdditionalInstanceExtensions();

        // After all the instance extensions are added,
        // create vulkan instance
        initVulkan();
        createDebugMessenger();   // Create a debugger for vulkan instance

        // Connect vulkan instance and window system
        createWindowAndSurface();

        // Pick a physical device for rendering
        pickPhysicalDevice();

        // Adds any required device extensions before create a logical device
        addAdditionalDeviceExtensions();
        createDeviceAndQueues();

        // create swapchain
        createSwapchain();
        createSwapchainImageView();

        // Graphics pipeline
        createRenderPass();
        createGraphicsPipeline();
        createSwapchainFramebuffer();

        // Program Loop
        programLoop();

        cleanup();
    }

private:
    VkResult vkResult{};

    VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;

    // All the Vulkan program related data
    struct VulkanProgramInfo
    {
        GLFWwindow *window = nullptr;

        VkInstance vulkanInstance = VK_NULL_HANDLE;

        VkPhysicalDevice GPU = VK_NULL_HANDLE;

        VkDevice renderDevice = VK_NULL_HANDLE;

        VkQueue graphicsQueue = VK_NULL_HANDLE;

        VkQueue presentQueue = VK_NULL_HANDLE;

        VkSurfaceKHR windowSurface = VK_NULL_HANDLE;

        VkSwapchainKHR swapchain;

        VkFormat swapchainImageFormat;
        VkColorSpaceKHR swapchainImageColorSpace;
        VkExtent2D swapchainExtent;

        std::vector<VkImage> swapchainImages;
        std::vector<VkImageView> swapchainImageViews;

        uint32_t graphicsQueueFamilyIndex = 0;
        bool graphicsQueueFound = false;

        uint32_t presentQueueFamilyIndex = 0;
        bool presentQueueFound = false;

        // Graphics Pipelines
        VkPipelineLayout pipelineLayout;

        VkRenderPass renderPass;

        VkPipeline graphicsPipeline;

        std::vector<VkFramebuffer> swapchainFramebuffers;

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
                        VK_KHR_SWAPCHAIN_EXTENSION_NAME
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

    void createDeviceAndQueues()
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

        // Next find a present queue
        VkBool32 presentSupport;
        for (std::size_t i = 0; i < queueFamilyPptCount; i++)
        {
            vkGetPhysicalDeviceSurfaceSupportKHR(vulkanProgramInfo.GPU,
                                                 i,
                                                 vulkanProgramInfo.windowSurface,
                                                 &presentSupport);
            if (presentSupport)
            {
                vulkanProgramInfo.presentQueueFamilyIndex = i;
                vulkanProgramInfo.presentQueueFound = true;
                break;
            }
        }

        if (presentSupport != VK_TRUE)
        {
            std::cout << "Cannot find a queue that supports presentation!" << std::endl;
            exit(-1);
        }

        if (vulkanProgramInfo.graphicsQueueFamilyIndex != vulkanProgramInfo.presentQueueFamilyIndex)
        {
            std::cout << "Graphics queue index and present queue index are not the same!";
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

        vkGetDeviceQueue(vulkanProgramInfo.renderDevice,
                         vulkanProgramInfo.presentQueueFamilyIndex,
                         0,
                         &vulkanProgramInfo.presentQueue);
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

    void createSwapchain()
    {
        VkSurfaceCapabilitiesKHR swapchainCapabilities;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vulkanProgramInfo.GPU,
                                                  vulkanProgramInfo.windowSurface,
                                                  &swapchainCapabilities);

        // Choose surface format
        VkSurfaceFormatKHR chosenSurfaceFormat;
        chosenSurfaceFormat = chooseSurfaceFormat();

        // Choose presentation mode
        VkPresentModeKHR swapchainPresentMode;
        swapchainPresentMode = choosePresentMode();

        // Choose swapchain extent
        VkExtent2D swapchainExtent;
        swapchainExtent = chooseSwapchainExtent();

        // Choose double buffer or triple buffer or whatever the minimum is
        uint32_t minSwapchainImage;
        minSwapchainImage = chooseSwapchainMinimumImage();

        // Choose image sharing mode
        VkSharingMode swapchainImageSharingMode;
        swapchainImageSharingMode = chooseImageSharingMode();

        // Fill the swapchain create info
        VkSwapchainCreateInfoKHR swapchainCreateInfo;
        swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        swapchainCreateInfo.pNext = nullptr;
        swapchainCreateInfo.minImageCount = minSwapchainImage;
        swapchainCreateInfo.imageSharingMode = swapchainImageSharingMode;
        swapchainCreateInfo.imageFormat = chosenSurfaceFormat.format;
        swapchainCreateInfo.imageColorSpace = chosenSurfaceFormat.colorSpace;
        swapchainCreateInfo.oldSwapchain = nullptr;
        swapchainCreateInfo.clipped = VK_TRUE;
        swapchainCreateInfo.presentMode = swapchainPresentMode;
        swapchainCreateInfo.preTransform = swapchainCapabilities.currentTransform;
        swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        swapchainCreateInfo.imageArrayLayers = 1;
        swapchainCreateInfo.imageExtent = swapchainExtent;
        swapchainCreateInfo.surface = vulkanProgramInfo.windowSurface;
        swapchainCreateInfo.pQueueFamilyIndices = &vulkanProgramInfo.presentQueueFamilyIndex;
        swapchainCreateInfo.queueFamilyIndexCount = 1;

        vkResult = vkCreateSwapchainKHR(vulkanProgramInfo.renderDevice,
                                        &swapchainCreateInfo,
                                        nullptr,
                                        &vulkanProgramInfo.swapchain);

        checkVkResult(vkResult, "Failed to create swapchain!");

        // Retrieve swapchain images
        uint32_t swapchainImageCount;
        vkGetSwapchainImagesKHR(vulkanProgramInfo.renderDevice,
                                vulkanProgramInfo.swapchain,
                                &swapchainImageCount,
                                nullptr);

        vulkanProgramInfo.swapchainImages.resize(swapchainImageCount);

        vkGetSwapchainImagesKHR(vulkanProgramInfo.renderDevice,
                                vulkanProgramInfo.swapchain,
                                &swapchainImageCount,
                                vulkanProgramInfo.swapchainImages.data());

        // Save this information for later use
        vulkanProgramInfo.swapchainImageFormat = swapchainCreateInfo.imageFormat;
        vulkanProgramInfo.swapchainImageColorSpace = swapchainCreateInfo.imageColorSpace;
        vulkanProgramInfo.swapchainExtent = swapchainCreateInfo.imageExtent;
    }

    void createSwapchainImageView()
    {
        vulkanProgramInfo.swapchainImageViews.resize(vulkanProgramInfo.swapchainImages.size());
        // First fill the image view create info except the image memeber
        VkImageViewCreateInfo imageViewCreateInfo;
        imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        imageViewCreateInfo.format = vulkanProgramInfo.swapchainImageFormat;
        imageViewCreateInfo.pNext = nullptr;
        imageViewCreateInfo.flags = 0;
        imageViewCreateInfo.subresourceRange.layerCount = 1;
        imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
        imageViewCreateInfo.subresourceRange.levelCount = 1;
        imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
        imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;

        for (std::size_t i = 0; i < vulkanProgramInfo.swapchainImages.size(); i++)
        {
            imageViewCreateInfo.image = vulkanProgramInfo.swapchainImages[i];
            vkCreateImageView(vulkanProgramInfo.renderDevice,
                              &imageViewCreateInfo,
                              nullptr,
                              &vulkanProgramInfo.swapchainImageViews[i]);
        }
    }

    void createGraphicsPipeline()
    {
        auto vertShaderCode = readFile("../src/spvShaders/vert.spv");
        auto fragShaderCode = readFile("../src/spvShaders/frag.spv");

        VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
        VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

        VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
        vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertShaderStageInfo.module = vertShaderModule;
        vertShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
        fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragShaderStageInfo.module = fragShaderModule;
        fragShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo,
                                                          fragShaderStageInfo};

        // ===========================================================================
        // Fixed functions
        // ===========================================================================

        // Vertex Input
        VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo{};
        vertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputStateCreateInfo.pNext = nullptr;
        vertexInputStateCreateInfo.pVertexAttributeDescriptions = nullptr;
        vertexInputStateCreateInfo.pVertexBindingDescriptions = nullptr;
        vertexInputStateCreateInfo.vertexAttributeDescriptionCount = 0;
        vertexInputStateCreateInfo.vertexBindingDescriptionCount = 0;

        // Input Assembly
        VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo{};
        inputAssemblyStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssemblyStateCreateInfo.primitiveRestartEnable = VK_FALSE;
        inputAssemblyStateCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

        // Viewport
        VkViewport viewport{};
        viewport.width = (float) vulkanProgramInfo.swapchainExtent.width;
        viewport.height = (float) vulkanProgramInfo.swapchainExtent.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        viewport.x = 0.0f;
        viewport.y = 0.0f;

        VkRect2D scissor{};
        scissor.extent = vulkanProgramInfo.swapchainExtent;
        scissor.offset.x = 0.0f;
        scissor.offset.y = 0.0f;

        VkPipelineViewportStateCreateInfo viewportStateCreateInfo{};
        viewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportStateCreateInfo.pScissors = &scissor;
        viewportStateCreateInfo.scissorCount = 1;
        viewportStateCreateInfo.pViewports = &viewport;
        viewportStateCreateInfo.viewportCount = 1;

        // Rasterizer
        VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo{};
        rasterizationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizationStateCreateInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
        rasterizationStateCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterizationStateCreateInfo.depthBiasEnable = VK_FALSE;
        rasterizationStateCreateInfo.depthClampEnable = VK_FALSE;
        rasterizationStateCreateInfo.lineWidth = 1.0f;
        rasterizationStateCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizationStateCreateInfo.rasterizerDiscardEnable = VK_FALSE;

        // Multisampling
        VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo{};
        multisampleStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampleStateCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        multisampleStateCreateInfo.sampleShadingEnable = VK_FALSE;

        // Depth and Stencil: Skip for now

        // Color blending
        VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo{};
        colorBlendStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlendStateCreateInfo.attachmentCount = 1;
        colorBlendStateCreateInfo.logicOpEnable = VK_FALSE;

        VkPipelineColorBlendAttachmentState colorBlendAttachmentState{};
        colorBlendAttachmentState.blendEnable = VK_FALSE;
        colorBlendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_A_BIT |
                                                   VK_COLOR_COMPONENT_R_BIT |
                                                   VK_COLOR_COMPONENT_G_BIT |
                                                   VK_COLOR_COMPONENT_B_BIT;

        colorBlendStateCreateInfo.pAttachments = &colorBlendAttachmentState;

        // Dynamic State: nullptr

        // Pipeline Layout
        VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
        pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutCreateInfo.pPushConstantRanges = nullptr;
        pipelineLayoutCreateInfo.pSetLayouts = nullptr;
        pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
        pipelineLayoutCreateInfo.setLayoutCount = 0;

        // END of setting fixed function state
        vkResult = vkCreatePipelineLayout(vulkanProgramInfo.renderDevice,
                                          &pipelineLayoutCreateInfo,
                                          nullptr,
                                          &vulkanProgramInfo.pipelineLayout);

        checkVkResult(vkResult, "Failed to Pipeline Layout");

        // Start creating graphics pipeline
        VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo{};
        graphicsPipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        graphicsPipelineCreateInfo.renderPass = vulkanProgramInfo.renderPass;
        graphicsPipelineCreateInfo.layout = vulkanProgramInfo.pipelineLayout;
        graphicsPipelineCreateInfo.pVertexInputState = &vertexInputStateCreateInfo;
        graphicsPipelineCreateInfo.pInputAssemblyState = &inputAssemblyStateCreateInfo;
        graphicsPipelineCreateInfo.pRasterizationState = &rasterizationStateCreateInfo;
        graphicsPipelineCreateInfo.pMultisampleState = &multisampleStateCreateInfo;
        graphicsPipelineCreateInfo.pColorBlendState = &colorBlendStateCreateInfo;
        graphicsPipelineCreateInfo.pDynamicState = nullptr;
        graphicsPipelineCreateInfo.pStages = shaderStages;
        graphicsPipelineCreateInfo.stageCount = 2;
        graphicsPipelineCreateInfo.pDepthStencilState = nullptr;
        graphicsPipelineCreateInfo.pViewportState = &viewportStateCreateInfo;
        graphicsPipelineCreateInfo.subpass = 0;

        vkCreateGraphicsPipelines(vulkanProgramInfo.renderDevice,
                                  VK_NULL_HANDLE,
                                  1,
                                  &graphicsPipelineCreateInfo,
                                  nullptr,
                                  &vulkanProgramInfo.graphicsPipeline);

        vkDestroyShaderModule(vulkanProgramInfo.renderDevice,
                              fragShaderModule, nullptr);
        vkDestroyShaderModule(vulkanProgramInfo.renderDevice,
                              vertShaderModule, nullptr);
    }

    void createRenderPass()
    {
        // Attachment description for render pass
        VkAttachmentDescription colorAttachment{};
        colorAttachment.format = vulkanProgramInfo.swapchainImageFormat;
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;

        // Attachment reference for subpass
        VkAttachmentReference colorAttachmentReference{};
        colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        colorAttachmentReference.attachment = 0;

        // Subpass description
        VkSubpassDescription subpassDescription{};
        subpassDescription.colorAttachmentCount = 1;
        subpassDescription.pColorAttachments = &colorAttachmentReference;
        subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

        // Render Pass create info
        VkRenderPassCreateInfo renderPassCreateInfo{};
        renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassCreateInfo.attachmentCount = 1;
        renderPassCreateInfo.pAttachments = &colorAttachment;
        renderPassCreateInfo.pSubpasses = &subpassDescription;
        renderPassCreateInfo.subpassCount = 1;

        vkResult = vkCreateRenderPass(vulkanProgramInfo.renderDevice,
                                      &renderPassCreateInfo,
                                      nullptr,
                                      &vulkanProgramInfo.renderPass);
        checkVkResult(vkResult, "Failed to create Render Pass");
    }

    void createSwapchainFramebuffer()
    {
        vulkanProgramInfo.swapchainFramebuffers.resize(vulkanProgramInfo.swapchainImages.size());

        // Create a Framebuffer for each image in swapchain
        for (std::size_t i = 0; i < vulkanProgramInfo.swapchainImages.size(); i++)
        {
            VkFramebufferCreateInfo framebufferCreateInfo{};
            framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferCreateInfo.renderPass = vulkanProgramInfo.renderPass;
            framebufferCreateInfo.pAttachments = &vulkanProgramInfo.swapchainImageViews[i];
            framebufferCreateInfo.attachmentCount = 1;
            framebufferCreateInfo.height = vulkanProgramInfo.swapchainExtent.height;
            framebufferCreateInfo.width  = vulkanProgramInfo.swapchainExtent.width;
            framebufferCreateInfo.layers = 1;

            vkResult = vkCreateFramebuffer(vulkanProgramInfo.renderDevice,
                                &framebufferCreateInfo,
                                nullptr,
                                &vulkanProgramInfo.swapchainFramebuffers[i]);

            checkVkResult(vkResult, "Failed to create Framebuffer");
        }
    }

    void programLoop()
    {
        while (!glfwWindowShouldClose(vulkanProgramInfo.window))
        {
            glfwPollEvents();
        }
    }

    void cleanup() const
    {
        for (const VkFramebuffer &swapchainFramebuffer : vulkanProgramInfo.swapchainFramebuffers)
        {
            vkDestroyFramebuffer(vulkanProgramInfo.renderDevice,
                                 swapchainFramebuffer,
                                 nullptr);
        }

        vkDestroyPipeline(vulkanProgramInfo.renderDevice,
                          vulkanProgramInfo.graphicsPipeline,
                          nullptr);

        vkDestroyPipelineLayout(vulkanProgramInfo.renderDevice,
                                vulkanProgramInfo.pipelineLayout,
                                nullptr);
        vkDestroyRenderPass(vulkanProgramInfo.renderDevice,
                            vulkanProgramInfo.renderPass,
                            nullptr);
        for (const auto &imageView: vulkanProgramInfo.swapchainImageViews)
        {
            vkDestroyImageView(vulkanProgramInfo.renderDevice,
                               imageView,
                               nullptr);
        }
        vkDestroySwapchainKHR(vulkanProgramInfo.renderDevice,
                              vulkanProgramInfo.swapchain,
                              nullptr);

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
        const char **glfwInstanceExtensions;
        glfwInstanceExtensions = glfwGetRequiredInstanceExtensions(&glfwRequiredExtensionCount);

        if (glfwInstanceExtensions == nullptr)
        {
            std::cout << "Failed to get instance extensions required by glfw" << std::endl;
            exit(-1);
        }

        for (std::size_t i = 0; i < glfwRequiredExtensionCount; i++)
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

    /*
     * ============================================================
     * START: Swapchain Helper Functions
     * ============================================================
     */
    VkSurfaceFormatKHR chooseSurfaceFormat()
    {
        uint32_t surfaceFormatCount = 0;
        vkGetPhysicalDeviceSurfaceFormatsKHR(vulkanProgramInfo.GPU,
                                             vulkanProgramInfo.windowSurface,
                                             &surfaceFormatCount,
                                             nullptr);

        std::vector<VkSurfaceFormatKHR> surfaceFormats(surfaceFormatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(vulkanProgramInfo.GPU,
                                             vulkanProgramInfo.windowSurface,
                                             &surfaceFormatCount,
                                             surfaceFormats.data());

        for (std::size_t i = 0; i < surfaceFormatCount; i++)
        {
            // Pick VK_FORMAT_B8G8R8A8_SRGB if it is available
            if (surfaceFormats[i].format == VK_FORMAT_B8G8R8A8_SRGB &&
                surfaceFormats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            {
                return surfaceFormats[i];
            }
        }

        // If VK_FORMAT_B8G8R8A8_SRGB is not available just pick the first one
        return surfaceFormats[0];
    }

    VkPresentModeKHR choosePresentMode()
    {
        uint32_t presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(vulkanProgramInfo.GPU,
                                                  vulkanProgramInfo.windowSurface,
                                                  &presentModeCount,
                                                  nullptr);

        std::vector<VkPresentModeKHR> presentModes(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(vulkanProgramInfo.GPU,
                                                  vulkanProgramInfo.windowSurface,
                                                  &presentModeCount,
                                                  presentModes.data());

        for (std::size_t i = 0; i < presentModeCount; i++)
        {
            if (presentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
            {
                return presentModes[i];
            }
        }

        return VK_PRESENT_MODE_FIFO_KHR;
    }

    VkExtent2D chooseSwapchainExtent()
    {
        VkSurfaceCapabilitiesKHR surfaceCapabilities;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vulkanProgramInfo.GPU,
                                                  vulkanProgramInfo.windowSurface,
                                                  &surfaceCapabilities);

        // Test if there is a limitation in image extent
        if (surfaceCapabilities.currentExtent.height != UINT32_MAX)
        {
            return surfaceCapabilities.currentExtent;
        }
            // If there is no limitation, choose a size matches the window size
        else
        {
            int windowWidthPixel, windowHeightPixel;
            // because glfw uses different unit
            glfwGetFramebufferSize(vulkanProgramInfo.window,
                                   &windowWidthPixel,
                                   &windowHeightPixel);

            VkExtent2D finalExtent;
            finalExtent.height = std::clamp((uint32_t) windowHeightPixel,
                                            surfaceCapabilities.minImageExtent.height,
                                            surfaceCapabilities.maxImageExtent.height);

            finalExtent.width = std::clamp((uint32_t) windowWidthPixel,
                                           surfaceCapabilities.minImageExtent.width,
                                           surfaceCapabilities.maxImageExtent.width);

            return finalExtent;
        }
    }

    uint32_t chooseSwapchainMinimumImage()
    {
        VkSurfaceCapabilitiesKHR surfaceCapabilities;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vulkanProgramInfo.GPU,
                                                  vulkanProgramInfo.windowSurface,
                                                  &surfaceCapabilities);

        // If maxImageCount is 0, there is no limit in number of images in swapchain
        if (surfaceCapabilities.maxImageCount == 0)
        {
            if (surfaceCapabilities.minImageCount <= 2)
            {
                return 2;
            } else
            {
                return surfaceCapabilities.minImageCount;
            }
        } else
        {
            return std::clamp((uint32_t) 2,
                              surfaceCapabilities.minImageCount,
                              surfaceCapabilities.maxImageCount);
        }
    }

    VkSharingMode chooseImageSharingMode()
    {
        // Because currently we only deal with present queue and graphics queue that are the same
        return VK_SHARING_MODE_EXCLUSIVE;
    }

    /*
     * ============================================================
     * END: Swapchain Helper Functions
     * ============================================================
     */

    VkShaderModule createShaderModule(const std::vector<char> &code)
    {
        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = code.size();
        createInfo.pCode = reinterpret_cast<const uint32_t *>(code.data());

        VkShaderModule shaderModule;
        if (vkCreateShaderModule(vulkanProgramInfo.renderDevice,
                                 &createInfo, nullptr,
                                 &shaderModule) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create shader module!");
        }

        return shaderModule;
    }

    static std::vector<char> readFile(const std::string &filename)
    {
        std::ifstream file(filename, std::ios::ate | std::ios::binary);

        if (!file.is_open())
        {
            throw std::runtime_error("failed to open file!");
        }

        size_t fileSize = (size_t) file.tellg();
        std::vector<char> buffer(fileSize);

        file.seekg(0);
        file.read(buffer.data(), fileSize);

        file.close();

        return buffer;
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
