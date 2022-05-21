#define  GLM_FORCE_RADIANS
#define  GLM_FORCE_DEPTH_ZERO_TO_ONE
#define  TINYOBJLOADER_IMPLEMENTATION
#define  STB_IMAGE_IMPLEMENTATION

#include <array>
#include <ios>
#include <stdexcept>
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <cstdlib>
#include <fstream>
#include <cstring>
#include <algorithm>
#include "glm/ext/matrix_float4x4.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/ext/vector_float3.hpp"
#include "vertex.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <chrono>
#include "vulkan/vulkan_core.h"
#include "vulkan_helpers.h"
#include <stb_image.h>
#include <tiny_obj_loader.h>

const uint32_t windowWidth = 800;
const uint32_t windowHeight = 800;
const int MAX_FRAMES_IN_FLIGHT = 2;
const std::string mesh_path = "../src/meshes/mesh.obj";
const std::string mesh_texture_path = "../src/meshes/mesh_pic.png";

std::vector<struct Vertex> vertices;
std::vector<uint32_t> vertex_indices;

float direction = 0;

void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_E && (action == GLFW_PRESS || action == GLFW_REPEAT))
    {
        direction += 0.01f;
    } else if (key == GLFW_KEY_Y && (action == GLFW_PRESS || action == GLFW_REPEAT))
    {
        direction -= 0.01f;
    }
}

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

        // glfw key inputs
        glfwSetKeyCallback(vulkanProgramInfo.window, keyCallback);

        // Pick a physical device for rendering
        pickPhysicalDevice();

        // Adds any required device extensions before create a logical device
        addAdditionalDeviceExtensions();
        createDeviceAndQueues();

        // create swapchain
        createSwapchain();
        createSwapchainImageView();

        // Drawing Commands
        createCommandBuffers();

        // Depth buffer
        createDepthBuffer();

        // Texture Images and its Image View
        createTextureImage();
        createTextureImageView();

        // Uniform buffers
        // Create Descriptor set which contains uniform buffers
        createDescriptorSetLayout();
        createUniformBuffer();
        createDescriptorPool();

        // Preparing for graphics pipeline
        // Create vertex buffer
        loadModel();
        createVertexBufferAndAllocateMemory();

        // Create vertex index buffer
        createIndexBuffer();

        // Graphics pipeline
        createRenderPass();
        createGraphicsPipeline();

        createSwapchainFramebuffer();


        // Synchronization Objects
        createSynchronizationObjects();


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

        uint32_t activeSwapchainImage = 0;

        std::vector<VkFramebuffer> swapchainFramebuffers;

        // Command Buffers
        VkCommandPool commandPool;
        VkCommandBuffer commandBuffers[MAX_FRAMES_IN_FLIGHT];

        // Synchronization Objects
        VkFence frameReadyFences[MAX_FRAMES_IN_FLIGHT];
        VkSemaphore nextImageReadySemaphores[MAX_FRAMES_IN_FLIGHT];
        VkSemaphore renderFinishedSemaphores[MAX_FRAMES_IN_FLIGHT];

        // Keep track of current frame. Its value should never exceed MAX_IN_FLIGHT_FRAMES
        int curr_frame = 0;

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

        VkBuffer vertexBuffer = VK_NULL_HANDLE;
        VkDeviceMemory vertexBufferMemory = VK_NULL_HANDLE;
        VkBuffer indexBuffer = VK_NULL_HANDLE;
        VkDeviceMemory indexBufferMemory = VK_NULL_HANDLE;

        std::vector<VkBuffer> uniformBuffers;
        std::vector<VkDeviceMemory> uniformBufferMemories;
        VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
        VkDescriptorPool descriptorPool;
        std::vector<VkDescriptorSet> descriptorSets;

        // Texture Images
        VkImage textureImage;
        VkDeviceMemory textureImageMemory;
        VkImageView textureImageView;


        // Image Sampler
        VkSampler textureImageSampler = VK_NULL_HANDLE;

        // The TRIFORCE !!!
        VkImage depthImage = VK_NULL_HANDLE;
        VkDeviceMemory depthImageMemory = VK_NULL_HANDLE;
        VkImageView depthImageView = VK_NULL_HANDLE;

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

    void createTextureImage()
    {
        int texWidth, texHeight, texChannels;
        stbi_uc *pixels = stbi_load(mesh_texture_path.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
        VkDeviceSize imageSize = texWidth * texHeight * 4;

        if (pixels == nullptr)
        {
            throw std::runtime_error("failed to read pixels from image");
        }

        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;

        VkBufferCreateInfo stagingBufferCreateInfo{};
        stagingBufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        stagingBufferCreateInfo.queueFamilyIndexCount = 0;
        stagingBufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        stagingBufferCreateInfo.size = imageSize;
        stagingBufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

        vkCreateBuffer(vulkanProgramInfo.renderDevice,
                       &stagingBufferCreateInfo,
                       nullptr,
                       &stagingBuffer);

        VkMemoryRequirements stagingBufferMemoryRequirements{};
        vkGetBufferMemoryRequirements(vulkanProgramInfo.renderDevice,
                                      stagingBuffer,
                                      &stagingBufferMemoryRequirements);

        VkPhysicalDeviceMemoryProperties physicalDeviceMemoryProperties;
        vkGetPhysicalDeviceMemoryProperties(vulkanProgramInfo.GPU,
                                            &physicalDeviceMemoryProperties);

        VkMemoryPropertyFlags suitableMemoryProperty = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                                       VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

        VkMemoryAllocateInfo stagingBufferMemoryAllocateInfo{};
        stagingBufferMemoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;

        bool foundSuitableMemoryType = false;

        for (int i = 0; i < physicalDeviceMemoryProperties.memoryTypeCount; i++)
        {
            VkMemoryPropertyFlags currMemoryTypePropertyFlag =
                    physicalDeviceMemoryProperties.memoryTypes[i].propertyFlags;

            if ((stagingBufferMemoryRequirements.memoryTypeBits & (1 << i))
                &&
                (currMemoryTypePropertyFlag & suitableMemoryProperty) == suitableMemoryProperty)
            {
                stagingBufferMemoryAllocateInfo.memoryTypeIndex = i;
                foundSuitableMemoryType = true;
                break;
            }
        }

        if (!foundSuitableMemoryType)
        {
            throw std::runtime_error("Failed to find suitable memory type");
        }

        stagingBufferMemoryAllocateInfo.allocationSize = stagingBufferMemoryRequirements.size;
        stagingBufferMemoryAllocateInfo.pNext = nullptr;

        vkAllocateMemory(vulkanProgramInfo.renderDevice,
                         &stagingBufferMemoryAllocateInfo,
                         nullptr,
                         &stagingBufferMemory);

        vkBindBufferMemory(vulkanProgramInfo.renderDevice,
                           stagingBuffer,
                           stagingBufferMemory,
                           0);

        void *stagingBufferMappedMemory;
        vkMapMemory(vulkanProgramInfo.renderDevice,
                    stagingBufferMemory,
                    0,
                    imageSize,
                    0,
                    &stagingBufferMappedMemory);
        memcpy(stagingBufferMappedMemory, pixels, static_cast<size_t>(imageSize));
        vkUnmapMemory(vulkanProgramInfo.renderDevice, stagingBufferMemory);


        // The above is for creating a staging buffer where its content will be transferred to an image obejct
        // Next creating the image object as the recieving end of the image texture data


        VkImageCreateInfo textureImageCreateInfo{};
        textureImageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        textureImageCreateInfo.arrayLayers = 1;
        textureImageCreateInfo.extent.depth = 1;
        textureImageCreateInfo.extent.height = texHeight;
        textureImageCreateInfo.extent.width = texWidth;
        textureImageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
        textureImageCreateInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
        textureImageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        textureImageCreateInfo.mipLevels = 1;
        textureImageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        textureImageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        textureImageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;

        vkCreateImage(vulkanProgramInfo.renderDevice, &textureImageCreateInfo, nullptr,
                      &vulkanProgramInfo.textureImage);

        VkMemoryRequirements texImageMemoryRequirements;
        vkGetImageMemoryRequirements(vulkanProgramInfo.renderDevice,
                                     vulkanProgramInfo.textureImage,
                                     &texImageMemoryRequirements);

        VkMemoryAllocateInfo textureImageMemoryAllocateInfo{};
        textureImageMemoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        textureImageMemoryAllocateInfo.allocationSize = texImageMemoryRequirements.size;
        textureImageMemoryAllocateInfo.pNext = nullptr;

        VkMemoryPropertyFlags suitableTexImageMemoryProperty = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

        foundSuitableMemoryType = false;
        for (int i = 0; i < physicalDeviceMemoryProperties.memoryTypeCount; i++)
        {
            VkMemoryPropertyFlags currMemoryTypePropertyFlag
                    = physicalDeviceMemoryProperties.memoryTypes[i].propertyFlags;
            if (texImageMemoryRequirements.memoryTypeBits & (1 << i)
                &&
                (currMemoryTypePropertyFlag & suitableTexImageMemoryProperty) == suitableTexImageMemoryProperty)
            {
                textureImageMemoryAllocateInfo.memoryTypeIndex = i;
                foundSuitableMemoryType = true;
            }
        }

        if (!foundSuitableMemoryType)
        {
            throw std::runtime_error("Failed to find suitable memory for texture image");
        }

        vkAllocateMemory(vulkanProgramInfo.renderDevice, &textureImageMemoryAllocateInfo, nullptr,
                         &vulkanProgramInfo.textureImageMemory);
        vkBindImageMemory(vulkanProgramInfo.renderDevice, vulkanProgramInfo.textureImage,
                          vulkanProgramInfo.textureImageMemory, 0);

        // Right here the image staing buffer is created and the destination image buffer is waiting for data
        // to be copied from staging buffer.

        // But before copying started, image needs to be in transfer layout
        VkImageMemoryBarrier imageBarrierToTransfer{};
        imageBarrierToTransfer.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        imageBarrierToTransfer.image = vulkanProgramInfo.textureImage;
        imageBarrierToTransfer.srcAccessMask = 0;
        imageBarrierToTransfer.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        imageBarrierToTransfer.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imageBarrierToTransfer.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imageBarrierToTransfer.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageBarrierToTransfer.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        imageBarrierToTransfer.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageBarrierToTransfer.subresourceRange.baseArrayLayer = 0;
        imageBarrierToTransfer.subresourceRange.baseMipLevel = 0;
        imageBarrierToTransfer.subresourceRange.layerCount = 1;
        imageBarrierToTransfer.subresourceRange.levelCount = 1;

        VkCommandBuffer transferImageLayoutForCopyingBuffer{};
        VkCommandBufferAllocateInfo transferImageLayoutForCopyingBufferCreateInfo{};
        transferImageLayoutForCopyingBufferCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        transferImageLayoutForCopyingBufferCreateInfo.commandPool = vulkanProgramInfo.commandPool;
        transferImageLayoutForCopyingBufferCreateInfo.commandBufferCount = 1;
        transferImageLayoutForCopyingBufferCreateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

        vkResult = vkAllocateCommandBuffers(vulkanProgramInfo.renderDevice,
                                            &transferImageLayoutForCopyingBufferCreateInfo,
                                            &transferImageLayoutForCopyingBuffer);
        checkVkResult(vkResult, "Failed to allocate command buffer");

        VkCommandBufferBeginInfo transferImageLayoutForCopyingBufferBeginInfo{};
        transferImageLayoutForCopyingBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        transferImageLayoutForCopyingBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        transferImageLayoutForCopyingBufferBeginInfo.pInheritanceInfo = nullptr;

        vkResult = vkBeginCommandBuffer(transferImageLayoutForCopyingBuffer,
                                        &transferImageLayoutForCopyingBufferBeginInfo);
        checkVkResult(vkResult, "Failed to begin buffer");

        vkCmdPipelineBarrier(transferImageLayoutForCopyingBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                             VK_PIPELINE_STAGE_TRANSFER_BIT,
                             0,
                             0, nullptr,
                             0, nullptr,
                             1, &imageBarrierToTransfer);

        vkEndCommandBuffer(transferImageLayoutForCopyingBuffer);

        VkSubmitInfo transferImageLayoutForCopyingBufferSubmitInfo{};
        transferImageLayoutForCopyingBufferSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        transferImageLayoutForCopyingBufferSubmitInfo.pCommandBuffers = &transferImageLayoutForCopyingBuffer;
        transferImageLayoutForCopyingBufferSubmitInfo.commandBufferCount = 1;

        vkQueueSubmit(vulkanProgramInfo.graphicsQueue, 1, &transferImageLayoutForCopyingBufferSubmitInfo,
                      VK_NULL_HANDLE);
        vkQueueWaitIdle(vulkanProgramInfo.graphicsQueue);

        vkFreeCommandBuffers(vulkanProgramInfo.renderDevice, vulkanProgramInfo.commandPool, 1,
                             &transferImageLayoutForCopyingBuffer);

        // Above has done creating command buffer for layout transition for image recieving, create image barrier,
        // recording barrier to pipeline and free command buffer
        // Now the image layout is set for recieving all those texel data, copy those image data from staing buffer
        // to our Vulkan image object.
        VkCommandBuffer copyImageCommandBuffer;

        VkCommandBufferAllocateInfo copyBufferCmdAllocateInfo{};
        copyBufferCmdAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        copyBufferCmdAllocateInfo.commandPool = vulkanProgramInfo.commandPool;
        copyBufferCmdAllocateInfo.commandBufferCount = 1;
        copyBufferCmdAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        copyBufferCmdAllocateInfo.pNext = nullptr;

        vkResult = vkAllocateCommandBuffers(vulkanProgramInfo.renderDevice, &copyBufferCmdAllocateInfo,
                                            &copyImageCommandBuffer);
        checkVkResult(vkResult, "Failed to create command buffer");

        // After buffer is available, begin command buffer
        VkCommandBufferBeginInfo copyImageBufferBeginInfo{};
        copyImageBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        copyImageBufferBeginInfo.pInheritanceInfo = nullptr;
        copyImageBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        vkBeginCommandBuffer(copyImageCommandBuffer, &copyImageBufferBeginInfo);

        VkBufferImageCopy texImageCopyRegion{};
        texImageCopyRegion.bufferOffset = 0;
        texImageCopyRegion.bufferRowLength = 0;
        texImageCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        texImageCopyRegion.imageSubresource.baseArrayLayer = 0;
        texImageCopyRegion.imageSubresource.layerCount = 1;
        texImageCopyRegion.imageSubresource.mipLevel = 0;
        texImageCopyRegion.imageOffset.x = 0;
        texImageCopyRegion.imageOffset.y = 0;
        texImageCopyRegion.imageOffset.z = 0;
        texImageCopyRegion.imageExtent = {static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight), 1};

        vkCmdCopyBufferToImage(copyImageCommandBuffer,
                               stagingBuffer,
                               vulkanProgramInfo.textureImage,
                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                               1, &texImageCopyRegion);

        vkEndCommandBuffer(copyImageCommandBuffer);

        VkSubmitInfo copyImageBufferSubmitInfo{};
        copyImageBufferSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        copyImageBufferSubmitInfo.commandBufferCount = 1;
        copyImageBufferSubmitInfo.pCommandBuffers = &copyImageCommandBuffer;

        vkQueueSubmit(vulkanProgramInfo.graphicsQueue, 1, &copyImageBufferSubmitInfo, VK_NULL_HANDLE);

        vkQueueWaitIdle(vulkanProgramInfo.graphicsQueue);

        // Clean copy image command buffer
        vkFreeCommandBuffers(vulkanProgramInfo.renderDevice, vulkanProgramInfo.commandPool, 1, &copyImageCommandBuffer);
        // Destroy staging buffer and deallocate its memory
        stbi_image_free(pixels);
        vkDestroyBuffer(vulkanProgramInfo.renderDevice,
                        stagingBuffer,
                        nullptr);

        vkFreeMemory(vulkanProgramInfo.renderDevice, stagingBufferMemory, nullptr);

        // At this stage, Vulkan image sucessfully got its content ready. We just need to prepare its layout to be
        // shader read ready.
        VkImageMemoryBarrier transitionToShaderReadBarrier{};
        transitionToShaderReadBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        transitionToShaderReadBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        transitionToShaderReadBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        transitionToShaderReadBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        transitionToShaderReadBarrier.image = vulkanProgramInfo.textureImage;
        transitionToShaderReadBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        transitionToShaderReadBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        transitionToShaderReadBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        transitionToShaderReadBarrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT,
                                                          0,
                                                          1,
                                                          0,
                                                          1};

        VkCommandBuffer transitionToShaderRead;
        VkCommandBufferAllocateInfo transitionToShaderReadAllocateInfo{};
        transitionToShaderReadAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        transitionToShaderReadAllocateInfo.commandPool = vulkanProgramInfo.commandPool;
        transitionToShaderReadAllocateInfo.commandBufferCount = 1;
        transitionToShaderReadAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

        vkResult = vkAllocateCommandBuffers(vulkanProgramInfo.renderDevice, &transitionToShaderReadAllocateInfo,
                                            &transitionToShaderRead);
        checkVkResult(vkResult, "Failed to allocate Command buffer");

        VkCommandBufferBeginInfo transitionToShaderReadBeginInfo
                {
                        VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
                        nullptr,
                        VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
                        nullptr
                };

        vkResult = vkBeginCommandBuffer(transitionToShaderRead, &transitionToShaderReadBeginInfo);
        checkVkResult(vkResult, "Failed to begin");

        vkCmdPipelineBarrier(transitionToShaderRead,
                             VK_PIPELINE_STAGE_TRANSFER_BIT,
                             VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                             0,
                             0, nullptr,
                             0, nullptr,
                             1, &transitionToShaderReadBarrier);


        vkEndCommandBuffer(transitionToShaderRead);

        VkSubmitInfo transitionToShaderReadSubmitInfo
                {
                        VK_STRUCTURE_TYPE_SUBMIT_INFO,
                        nullptr,
                        0,
                        nullptr,
                        nullptr,
                        1,
                        &transitionToShaderRead,
                        0,
                        nullptr
                };

        vkResult = vkQueueSubmit(vulkanProgramInfo.graphicsQueue, 1, &transitionToShaderReadSubmitInfo, VK_NULL_HANDLE);
        checkVkResult(vkResult, "Failed to submit");

        vkQueueWaitIdle(vulkanProgramInfo.graphicsQueue);

        // Free and destroy
        vkFreeCommandBuffers(vulkanProgramInfo.renderDevice, vulkanProgramInfo.commandPool, 1, &transitionToShaderRead);
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

        // Enable device features
        VkPhysicalDeviceFeatures enabledPhysicalDeviceFeatures{};
        enabledPhysicalDeviceFeatures.samplerAnisotropy = VK_TRUE;

        // Logical device creat info
        VkDeviceCreateInfo renderDeviceCreateInfo{};
        renderDeviceCreateInfo.flags = 0;
        renderDeviceCreateInfo.pNext = nullptr;
        renderDeviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        renderDeviceCreateInfo.enabledExtensionCount = vulkanProgramInfo.deviceExtensionsEnabled.size();
        renderDeviceCreateInfo.ppEnabledExtensionNames = vulkanProgramInfo.deviceExtensionsEnabled.data();
        renderDeviceCreateInfo.enabledLayerCount = 0;
        renderDeviceCreateInfo.pEnabledFeatures = &enabledPhysicalDeviceFeatures;
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
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

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

    void createVertexBufferAndAllocateMemory()
    {
        VkDeviceSize vertexBufferSize = sizeof(vertices[0]) * vertices.size();

        VkBufferCreateInfo vertexBufferCreateInfo{};
        vertexBufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        vertexBufferCreateInfo.size = vertexBufferSize;
        vertexBufferCreateInfo.pNext = nullptr;
        vertexBufferCreateInfo.flags = 0;
        vertexBufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        vertexBufferCreateInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

        VkBufferCreateInfo stagingBufferCreateInfo{};
        stagingBufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        stagingBufferCreateInfo.size = vertexBufferSize;
        stagingBufferCreateInfo.pNext = nullptr;
        stagingBufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        stagingBufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        createBuffer(vertexBufferCreateInfo,
                     vulkanProgramInfo.renderDevice,
                     vulkanProgramInfo.vertexBuffer,
                     vulkanProgramInfo.GPU,
                     vulkanProgramInfo.vertexBufferMemory,
                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        VkBuffer vertexStagingBuffer;
        VkDeviceMemory vertexStagingBufferMemory;

        createBuffer(stagingBufferCreateInfo,
                     vulkanProgramInfo.renderDevice,
                     vertexStagingBuffer,
                     vulkanProgramInfo.GPU,
                     vertexStagingBufferMemory,
                     VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

        void *stagingBufferMemoryMapping = nullptr;

        vkMapMemory(vulkanProgramInfo.renderDevice,
                    vertexStagingBufferMemory,
                    0,
                    vertexBufferCreateInfo.size,
                    0,
                    &stagingBufferMemoryMapping);

        memcpy(stagingBufferMemoryMapping,
               vertices.data(),
               vertexBufferCreateInfo.size);

        vkUnmapMemory(vulkanProgramInfo.renderDevice,
                      vertexStagingBufferMemory);

        // Copy buffer
        VkCommandBufferAllocateInfo copyBufferCmdCreateInfo{};
        copyBufferCmdCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        copyBufferCmdCreateInfo.commandBufferCount = 1;
        copyBufferCmdCreateInfo.commandPool = vulkanProgramInfo.commandPool;
        copyBufferCmdCreateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

        VkCommandBuffer copyBufferCmd;

        vkAllocateCommandBuffers(vulkanProgramInfo.renderDevice,
                                 &copyBufferCmdCreateInfo,
                                 &copyBufferCmd);

        VkCommandBufferBeginInfo copyBufferCmdBeginInfo{};
        copyBufferCmdBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        copyBufferCmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        vkBeginCommandBuffer(copyBufferCmd, &copyBufferCmdBeginInfo);

        VkBufferCopy vertexBufferCopy{};
        vertexBufferCopy.size = vertexBufferSize;
        vertexBufferCopy.dstOffset = 0;
        vertexBufferCopy.srcOffset = 0;

        vkCmdCopyBuffer(copyBufferCmd,
                        vertexStagingBuffer,
                        vulkanProgramInfo.vertexBuffer,
                        1,
                        &vertexBufferCopy);

        vkEndCommandBuffer(copyBufferCmd);

        VkSubmitInfo copyBufferCommandSubmitInfo{};
        copyBufferCommandSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        copyBufferCommandSubmitInfo.commandBufferCount = 1;
        copyBufferCommandSubmitInfo.pCommandBuffers = &copyBufferCmd;

        vkQueueSubmit(vulkanProgramInfo.graphicsQueue,
                      1,
                      &copyBufferCommandSubmitInfo,
                      VK_NULL_HANDLE);

        vkQueueWaitIdle(vulkanProgramInfo.graphicsQueue);

        vkFreeMemory(vulkanProgramInfo.renderDevice,
                     vertexStagingBufferMemory,
                     nullptr);

        vkDestroyBuffer(vulkanProgramInfo.renderDevice,
                        vertexStagingBuffer,
                        nullptr);
    }

    void createIndexBuffer()
    {
        VkDeviceSize indexBufferSize = sizeof(vertex_indices[0]) * vertex_indices.size();
        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;

        VkBufferCreateInfo indexBufferCreateInfo{};
        indexBufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        indexBufferCreateInfo.size = indexBufferSize;
        indexBufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        indexBufferCreateInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        createBuffer(indexBufferCreateInfo,
                     vulkanProgramInfo.renderDevice,
                     vulkanProgramInfo.indexBuffer,
                     vulkanProgramInfo.GPU,
                     vulkanProgramInfo.indexBufferMemory,
                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        VkBufferCreateInfo stagingBufferCreateInfo{};
        stagingBufferCreateInfo.size = indexBufferSize;
        stagingBufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        stagingBufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        stagingBufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createBuffer(stagingBufferCreateInfo,
                     vulkanProgramInfo.renderDevice,
                     stagingBuffer,
                     vulkanProgramInfo.GPU,
                     stagingBufferMemory,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        void *mappedMemoryData;
        vkMapMemory(vulkanProgramInfo.renderDevice,
                    stagingBufferMemory,
                    0,
                    indexBufferSize,
                    0,
                    &mappedMemoryData);
        memcpy(mappedMemoryData, vertex_indices.data(), indexBufferSize);
        vkUnmapMemory(vulkanProgramInfo.renderDevice,
                      stagingBufferMemory);

        VkCommandBuffer commandBuffer;

        VkCommandBufferAllocateInfo commandBufferAllocateInfo{};
        commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        commandBufferAllocateInfo.commandBufferCount = 1;
        commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        commandBufferAllocateInfo.commandPool = vulkanProgramInfo.commandPool;

        vkAllocateCommandBuffers(vulkanProgramInfo.renderDevice,
                                 &commandBufferAllocateInfo,
                                 &commandBuffer);

        VkCommandBufferBeginInfo commandBufferBeginInfo{};
        commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        VkBufferCopy bufferCopy{};
        bufferCopy.size = indexBufferSize;
        bufferCopy.srcOffset = 0;
        bufferCopy.dstOffset = 0;

        vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo);

        vkCmdCopyBuffer(commandBuffer, stagingBuffer, vulkanProgramInfo.indexBuffer, 1, &bufferCopy);

        vkEndCommandBuffer(commandBuffer);

        VkSubmitInfo commandBufferSubmitInfo{};
        commandBufferSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        commandBufferSubmitInfo.commandBufferCount = 1;
        commandBufferSubmitInfo.pCommandBuffers = &commandBuffer;

        vkQueueSubmit(vulkanProgramInfo.graphicsQueue,
                      1,
                      &commandBufferSubmitInfo,
                      VK_NULL_HANDLE);

        vkQueueWaitIdle(vulkanProgramInfo.graphicsQueue);

        vkFreeMemory(vulkanProgramInfo.renderDevice,
                     stagingBufferMemory,
                     nullptr);
        vkDestroyBuffer(vulkanProgramInfo.renderDevice,
                        stagingBuffer,
                        nullptr);
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

        // Vertex Input Bind Description
        VkVertexInputBindingDescription vertexInputBindingDescription{};
        vertexInputBindingDescription.binding = 1;
        vertexInputBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        vertexInputBindingDescription.stride = sizeof(Vertex);

        // Vertex Attribute Description
        VkVertexInputAttributeDescription vertexInputPosition{};
        vertexInputPosition.binding = 1;
        vertexInputPosition.location = 0;
        vertexInputPosition.format = VK_FORMAT_R32G32B32_SFLOAT;
        vertexInputPosition.offset = offsetof(Vertex, pos);

        VkVertexInputAttributeDescription vertexInputColor{};
        vertexInputColor.binding = 1;
        vertexInputColor.location = 1;
        vertexInputColor.format = VK_FORMAT_R32G32B32_SFLOAT;
        vertexInputColor.offset = offsetof(Vertex, color);

        VkVertexInputAttributeDescription vertexInputTexCoord{};
        vertexInputTexCoord.binding = 1;
        vertexInputTexCoord.location = 2;
        vertexInputTexCoord.format = VK_FORMAT_R32G32_SFLOAT;
        vertexInputTexCoord.offset = offsetof(Vertex, texCoord);

        VkVertexInputAttributeDescription vertexInputAttributeDescriptions[] =
                {
                        vertexInputPosition,
                        vertexInputColor,
                        vertexInputTexCoord
                };

        // Vertex Input
        VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo{};
        vertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputStateCreateInfo.pNext = nullptr;
        vertexInputStateCreateInfo.pVertexAttributeDescriptions = vertexInputAttributeDescriptions;
        vertexInputStateCreateInfo.pVertexBindingDescriptions = &vertexInputBindingDescription;
        vertexInputStateCreateInfo.vertexAttributeDescriptionCount =
                sizeof(vertexInputAttributeDescriptions) / sizeof(VkVertexInputAttributeDescription);;
        vertexInputStateCreateInfo.vertexBindingDescriptionCount = 1;

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
        rasterizationStateCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterizationStateCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

        // Depth and Stencil
        VkPipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo{};
        depthStencilStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencilStateCreateInfo.depthTestEnable = VK_TRUE;
        depthStencilStateCreateInfo.depthWriteEnable = VK_TRUE;
        depthStencilStateCreateInfo.depthCompareOp = VK_COMPARE_OP_LESS;
        depthStencilStateCreateInfo.depthBoundsTestEnable = VK_FALSE;
        depthStencilStateCreateInfo.stencilTestEnable = VK_FALSE;

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
        pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
        pipelineLayoutCreateInfo.pPushConstantRanges = nullptr;
        pipelineLayoutCreateInfo.setLayoutCount = 1;
        pipelineLayoutCreateInfo.pSetLayouts = &vulkanProgramInfo.descriptorSetLayout;

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
        graphicsPipelineCreateInfo.pDepthStencilState = &depthStencilStateCreateInfo;
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
        // Color attachment
        VkAttachmentDescription colorAttachment{};
        colorAttachment.format = vulkanProgramInfo.swapchainImageFormat;
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;

        // Depth attachment
        VkAttachmentDescription depthAttachment{};
        depthAttachment.format = VK_FORMAT_D32_SFLOAT;
        depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;

        // Attachment reference for subpass
        // Color attachment reference for our first and only subpass in the render pass
        VkAttachmentReference colorAttachmentReference{};
        colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        colorAttachmentReference.attachment = 0;

        // Depth attachment reference for our first and only subpass in the render pass
        VkAttachmentReference depthAttachmentReference{};
        depthAttachmentReference.attachment = 1;
        depthAttachmentReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        // Subpass description
        VkSubpassDescription subpassDescription{};
        subpassDescription.colorAttachmentCount = 1;
        subpassDescription.pColorAttachments = &colorAttachmentReference;
        subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpassDescription.pDepthStencilAttachment = &depthAttachmentReference;

        std::array<VkAttachmentDescription, 2> attachmentsForRenderPass =
                {
                        colorAttachment,
                        depthAttachment
                };

        // Render Pass create info
        VkRenderPassCreateInfo renderPassCreateInfo{};
        renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassCreateInfo.attachmentCount = static_cast<uint32_t>(attachmentsForRenderPass.size());
        renderPassCreateInfo.pAttachments = attachmentsForRenderPass.data();
        renderPassCreateInfo.pSubpasses = &subpassDescription;
        renderPassCreateInfo.subpassCount = 1;
        renderPassCreateInfo.dependencyCount = 0;
        renderPassCreateInfo.pDependencies = nullptr;

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

            std::array<VkImageView, 2> framebufferAttachments =
                    {
                            vulkanProgramInfo.swapchainImageViews[i],
                            vulkanProgramInfo.depthImageView
                    };
            VkFramebufferCreateInfo framebufferCreateInfo{};
            framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferCreateInfo.renderPass = vulkanProgramInfo.renderPass;
            framebufferCreateInfo.pAttachments = framebufferAttachments.data();
            framebufferCreateInfo.attachmentCount = static_cast<uint32_t>(framebufferAttachments.size());
            framebufferCreateInfo.height = vulkanProgramInfo.swapchainExtent.height;
            framebufferCreateInfo.width = vulkanProgramInfo.swapchainExtent.width;
            framebufferCreateInfo.layers = 1;

            vkResult = vkCreateFramebuffer(vulkanProgramInfo.renderDevice,
                                           &framebufferCreateInfo,
                                           nullptr,
                                           &vulkanProgramInfo.swapchainFramebuffers[i]);

            checkVkResult(vkResult, "Failed to create Framebuffer");
        }
    }

    void createCommandBuffers()
    {
        // Create a command pool
        VkCommandPoolCreateInfo commandPoolCreateInfo{};
        commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        commandPoolCreateInfo.queueFamilyIndex = vulkanProgramInfo.graphicsQueueFamilyIndex;

        vkCreateCommandPool(vulkanProgramInfo.renderDevice,
                            &commandPoolCreateInfo,
                            nullptr,
                            &vulkanProgramInfo.commandPool);

        // Allocate command buffer
        VkCommandBufferAllocateInfo commandBufferAllocateInfo{
                VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
                nullptr,
                vulkanProgramInfo.commandPool,
                VK_COMMAND_BUFFER_LEVEL_PRIMARY,
                (uint32_t) sizeof(vulkanProgramInfo.commandBuffers) / sizeof(VkCommandBuffer)
        };

        vkResult = vkAllocateCommandBuffers(vulkanProgramInfo.renderDevice,
                                            &commandBufferAllocateInfo,
                                            vulkanProgramInfo.commandBuffers);
        checkVkResult(vkResult, "Failed to allocate Command Buffers");
    }

    void recordCommandBuffer()
    {
        VkCommandBufferBeginInfo commandBufferBeginInfo{};
        commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        commandBufferBeginInfo.pInheritanceInfo = nullptr;

        VkClearValue colorClearValue{};
        colorClearValue.color = {0.0f, 0.0f, 0.0f, 1.0f};

        VkClearValue depthClearValue{};
        depthClearValue.depthStencil = {1.0f, 0};

        VkClearValue clearValues[2] =
                {
                        colorClearValue,
                        depthClearValue
                };

        VkRenderPassBeginInfo renderPassBeginInfo{};
        renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassBeginInfo.renderPass = vulkanProgramInfo.renderPass;
        renderPassBeginInfo.pClearValues = clearValues;
        renderPassBeginInfo.clearValueCount = 2;
        renderPassBeginInfo.framebuffer = vulkanProgramInfo.swapchainFramebuffers[vulkanProgramInfo.activeSwapchainImage];
        renderPassBeginInfo.renderArea.offset = {0, 0};
        renderPassBeginInfo.renderArea.extent = vulkanProgramInfo.swapchainExtent;

        vkBeginCommandBuffer(vulkanProgramInfo.commandBuffers[vulkanProgramInfo.curr_frame],
                             &commandBufferBeginInfo);

        vkCmdBindPipeline(vulkanProgramInfo.commandBuffers[vulkanProgramInfo.curr_frame],
                          VK_PIPELINE_BIND_POINT_GRAPHICS,
                          vulkanProgramInfo.graphicsPipeline);

        vkCmdBeginRenderPass(vulkanProgramInfo.commandBuffers[vulkanProgramInfo.curr_frame],
                             &renderPassBeginInfo,
                             VK_SUBPASS_CONTENTS_INLINE);

        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(vulkanProgramInfo.commandBuffers[vulkanProgramInfo.curr_frame],
                               1,
                               1,
                               &vulkanProgramInfo.vertexBuffer,
                               offsets);

        vkCmdBindIndexBuffer(vulkanProgramInfo.commandBuffers[vulkanProgramInfo.curr_frame],
                             vulkanProgramInfo.indexBuffer,
                             0,
                             VK_INDEX_TYPE_UINT32);
        /*
        vkCmdDraw(vulkanProgramInfo.commandBuffers[vulkanProgramInfo.curr_frame],
                  vertices.size(),
                  1,
                  0,
                  0);
        */
        vkCmdBindDescriptorSets(vulkanProgramInfo.commandBuffers[vulkanProgramInfo.curr_frame],
                                VK_PIPELINE_BIND_POINT_GRAPHICS,
                                vulkanProgramInfo.pipelineLayout,
                                0,
                                1,
                                &vulkanProgramInfo.descriptorSets[vulkanProgramInfo.curr_frame],
                                0,
                                nullptr);

        vkCmdDrawIndexed(vulkanProgramInfo.commandBuffers[vulkanProgramInfo.curr_frame],
                         static_cast<uint32_t>(vertex_indices.size()),
                         1,
                         0,
                         0,
                         0);

        vkCmdEndRenderPass(vulkanProgramInfo.commandBuffers[vulkanProgramInfo.curr_frame]);

        vkResult = vkEndCommandBuffer(vulkanProgramInfo.commandBuffers[vulkanProgramInfo.curr_frame]);

        checkVkResult(vkResult, "Failed to end command buffer");
    }

    void createSynchronizationObjects()
    {
        VkFenceCreateInfo frameReadyCreatInfo{};
        frameReadyCreatInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        frameReadyCreatInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        frameReadyCreatInfo.pNext = nullptr;

        VkSemaphoreCreateInfo nextImageReadySemaphoreCreateInfo{};
        nextImageReadySemaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkSemaphoreCreateInfo renderFinishedSemaphoreCreateInfo{};
        renderFinishedSemaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        for (std::size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            vkResult = vkCreateFence(vulkanProgramInfo.renderDevice,
                                     &frameReadyCreatInfo,
                                     nullptr,
                                     &vulkanProgramInfo.frameReadyFences[i]);

            checkVkResult(vkResult, "Failed to create frame ready fence");

            vkResult = vkCreateSemaphore(vulkanProgramInfo.renderDevice,
                                         &nextImageReadySemaphoreCreateInfo,
                                         nullptr,
                                         &vulkanProgramInfo.nextImageReadySemaphores[i]);
            checkVkResult(vkResult, "Failed to create next image ready semaphore");

            vkResult = vkCreateSemaphore(vulkanProgramInfo.renderDevice,
                                         &renderFinishedSemaphoreCreateInfo,
                                         nullptr,
                                         &vulkanProgramInfo.renderFinishedSemaphores[i]);

            checkVkResult(vkResult, "Failed to create render finished semaphore");
        }
    }

    void createDescriptorSetLayout()
    {
        VkDescriptorSetLayoutBinding descriptorSetLayoutBinding{};
        descriptorSetLayoutBinding.binding = 0;
        descriptorSetLayoutBinding.descriptorCount = 1;
        descriptorSetLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorSetLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

        VkDescriptorSetLayoutBinding textureImageSamplerDescriptorBinding{};
        textureImageSamplerDescriptorBinding.binding = 1;
        textureImageSamplerDescriptorBinding.descriptorCount = 1;
        textureImageSamplerDescriptorBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        textureImageSamplerDescriptorBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        std::array<VkDescriptorSetLayoutBinding, 2> descriptorSetLayoutBindings =
                {
                        descriptorSetLayoutBinding, textureImageSamplerDescriptorBinding
                };

        VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo{};
        descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        descriptorSetLayoutCreateInfo.bindingCount = static_cast<uint32_t>(descriptorSetLayoutBindings.size());
        descriptorSetLayoutCreateInfo.pBindings = descriptorSetLayoutBindings.data();

        vkCreateDescriptorSetLayout(vulkanProgramInfo.renderDevice,
                                    &descriptorSetLayoutCreateInfo,
                                    nullptr,
                                    &vulkanProgramInfo.descriptorSetLayout);
    }

    void createUniformBuffer()
    {
        vulkanProgramInfo.uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
        vulkanProgramInfo.uniformBufferMemories.resize(MAX_FRAMES_IN_FLIGHT);

        VkBufferCreateInfo uniformBufferCreateInfo{};
        uniformBufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        uniformBufferCreateInfo.size = sizeof(UniformBufferObject);
        uniformBufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        uniformBufferCreateInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            createBuffer(uniformBufferCreateInfo,
                         vulkanProgramInfo.renderDevice,
                         vulkanProgramInfo.uniformBuffers[i],
                         vulkanProgramInfo.GPU,
                         vulkanProgramInfo.uniformBufferMemories[i],
                         VK_MEMORY_PROPERTY_HOST_COHERENT_BIT |
                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

        }

    }

    void updateUniformBuffer()
    {
        // Copied from vulkan-tutorial.com
        static auto startTime = std::chrono::high_resolution_clock::now();

        auto currentTime = std::chrono::high_resolution_clock::now();
        float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

        UniformBufferObject ubo{};
        ubo.model = glm::rotate(glm::mat4(1.0f), 1.2f * direction, glm::vec3(0.0f, 0.0f, 1.0f));

        ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        ubo.proj = glm::perspective(glm::radians(45.0f),
                                    vulkanProgramInfo.swapchainExtent.width /
                                    (float) vulkanProgramInfo.swapchainExtent.height,
                                    0.1f,
                                    10.0f);
        ubo.proj[1][1] *= -1;

        // End of copy
        // I copied cuz I have no idea how to use glm or chrono lol

        void *uniformMappedMemory;
        vkMapMemory(vulkanProgramInfo.renderDevice,
                    vulkanProgramInfo.uniformBufferMemories[vulkanProgramInfo.curr_frame],
                    0,
                    sizeof(ubo),
                    0,
                    &uniformMappedMemory);

        memcpy(uniformMappedMemory, &ubo, sizeof(ubo));
        vkUnmapMemory(vulkanProgramInfo.renderDevice,
                      vulkanProgramInfo.uniformBufferMemories[vulkanProgramInfo.curr_frame]);
    }

    void createDescriptorPool()
    {
        VkDescriptorPoolSize uniformPoolSize{};
        uniformPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uniformPoolSize.descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

        VkDescriptorPoolSize samplerPoolSize{};
        samplerPoolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        samplerPoolSize.descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

        std::array<VkDescriptorPoolSize, 2> poolSizes =
                {
                        uniformPoolSize, samplerPoolSize
                };

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        poolInfo.pPoolSizes = poolSizes.data();
        poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

        vkResult = vkCreateDescriptorPool(vulkanProgramInfo.renderDevice,
                                          &poolInfo,
                                          nullptr,
                                          &vulkanProgramInfo.descriptorPool);

        checkVkResult(vkResult, "Failed to create Vulkan Descriptor Pool");

        std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, vulkanProgramInfo.descriptorSetLayout);
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = vulkanProgramInfo.descriptorPool;
        allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
        allocInfo.pSetLayouts = layouts.data();

        vulkanProgramInfo.descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
        if (vkAllocateDescriptorSets(vulkanProgramInfo.renderDevice, &allocInfo,
                                     vulkanProgramInfo.descriptorSets.data()) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to allocate descriptor sets!");
        }

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            VkDescriptorBufferInfo bufferInfo{};
            bufferInfo.buffer = vulkanProgramInfo.uniformBuffers[i];
            bufferInfo.offset = 0;
            bufferInfo.range = sizeof(UniformBufferObject);

            VkDescriptorImageInfo textureImageInfo{};
            textureImageInfo.imageView = vulkanProgramInfo.textureImageView;
            textureImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            textureImageInfo.sampler = vulkanProgramInfo.textureImageSampler;

            VkWriteDescriptorSet descriptorWriteUniformBuffer{};
            descriptorWriteUniformBuffer.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWriteUniformBuffer.dstSet = vulkanProgramInfo.descriptorSets[i];
            descriptorWriteUniformBuffer.dstBinding = 0;
            descriptorWriteUniformBuffer.dstArrayElement = 0;
            descriptorWriteUniformBuffer.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWriteUniformBuffer.descriptorCount = 1;
            descriptorWriteUniformBuffer.pBufferInfo = &bufferInfo;

            VkWriteDescriptorSet descriptorWriteImage{};
            descriptorWriteImage.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWriteImage.dstSet = vulkanProgramInfo.descriptorSets[i];
            descriptorWriteImage.dstBinding = 1;
            descriptorWriteImage.dstArrayElement = 0;
            descriptorWriteImage.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            descriptorWriteImage.descriptorCount = 1;
            descriptorWriteImage.pImageInfo = &textureImageInfo;

            std::array<VkWriteDescriptorSet, 2> descriptorWrites =
                    {
                            descriptorWriteUniformBuffer,
                            descriptorWriteImage
                    };

            vkUpdateDescriptorSets(vulkanProgramInfo.renderDevice,
                                   descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
        }
    }

    void drawFrame()
    {
        vkWaitForFences(vulkanProgramInfo.renderDevice,
                        1,
                        &vulkanProgramInfo.frameReadyFences[vulkanProgramInfo.curr_frame],
                        VK_TRUE,
                        UINT64_MAX);
        // Reset fence immediately for next use
        vkResetFences(vulkanProgramInfo.renderDevice,
                      1,
                      &vulkanProgramInfo.frameReadyFences[vulkanProgramInfo.curr_frame]);


        vkAcquireNextImageKHR(vulkanProgramInfo.renderDevice,
                              vulkanProgramInfo.swapchain,
                              UINT64_MAX,
                              vulkanProgramInfo.nextImageReadySemaphores[vulkanProgramInfo.curr_frame],
                              VK_NULL_HANDLE,
                              &vulkanProgramInfo.activeSwapchainImage);

        updateUniformBuffer();

        vkResetCommandBuffer(vulkanProgramInfo.commandBuffers[vulkanProgramInfo.curr_frame],
                             0);

        recordCommandBuffer();

        const VkPipelineStageFlags waitStageFlags[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

        VkSubmitInfo submitCmdBuffer{};
        submitCmdBuffer.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitCmdBuffer.waitSemaphoreCount = 1;
        submitCmdBuffer.pWaitSemaphores = &vulkanProgramInfo.nextImageReadySemaphores[vulkanProgramInfo.curr_frame];
        submitCmdBuffer.pWaitDstStageMask = waitStageFlags;
        submitCmdBuffer.commandBufferCount = 1;
        submitCmdBuffer.pCommandBuffers = &vulkanProgramInfo.commandBuffers[vulkanProgramInfo.curr_frame];
        submitCmdBuffer.signalSemaphoreCount = 1;
        submitCmdBuffer.pSignalSemaphores = &vulkanProgramInfo.renderFinishedSemaphores[vulkanProgramInfo.curr_frame];

        vkResult = vkQueueSubmit(vulkanProgramInfo.graphicsQueue,
                                 1,
                                 &submitCmdBuffer,
                                 vulkanProgramInfo.frameReadyFences[vulkanProgramInfo.curr_frame]);

        checkVkResult(vkResult, "Failed to submit command buffer");

        uint32_t renderedImageIndices[] = {vulkanProgramInfo.activeSwapchainImage};

        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = &vulkanProgramInfo.renderFinishedSemaphores[vulkanProgramInfo.curr_frame];
        presentInfo.pImageIndices = renderedImageIndices;
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &vulkanProgramInfo.swapchain;
        presentInfo.pResults = nullptr;

        vkQueuePresentKHR(vulkanProgramInfo.presentQueue,
                          &presentInfo);
    }

    void programLoop()
    {
        while (!glfwWindowShouldClose(vulkanProgramInfo.window))
        {
            glfwPollEvents();
            drawFrame();
            vulkanProgramInfo.curr_frame = (vulkanProgramInfo.curr_frame + 1) % MAX_FRAMES_IN_FLIGHT;
        }
        vkDeviceWaitIdle(vulkanProgramInfo.renderDevice);
    }

    void cleanup() const
    {
        vkDestroyImage(vulkanProgramInfo.renderDevice,
                       vulkanProgramInfo.depthImage,
                       nullptr);
        vkDestroyImageView(vulkanProgramInfo.renderDevice,
                           vulkanProgramInfo.depthImageView,
                           nullptr);
        vkFreeMemory(vulkanProgramInfo.renderDevice, vulkanProgramInfo.depthImageMemory, nullptr);

        vkDestroySampler(vulkanProgramInfo.renderDevice, vulkanProgramInfo.textureImageSampler, nullptr);

        // Destroy image and free image memory afterwards
        vkDestroyImageView(vulkanProgramInfo.renderDevice, vulkanProgramInfo.textureImageView, nullptr);
        vkDestroyImage(vulkanProgramInfo.renderDevice, vulkanProgramInfo.textureImage, nullptr);
        vkFreeMemory(vulkanProgramInfo.renderDevice, vulkanProgramInfo.textureImageMemory, nullptr);

        vkDestroyDescriptorPool(vulkanProgramInfo.renderDevice,
                                vulkanProgramInfo.descriptorPool,
                                nullptr);

        // Uniform buffers
        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            vkDestroyBuffer(vulkanProgramInfo.renderDevice,
                            vulkanProgramInfo.uniformBuffers[i],
                            nullptr);

            vkFreeMemory(vulkanProgramInfo.renderDevice,
                         vulkanProgramInfo.uniformBufferMemories[i],
                         nullptr);
        }
        vkDestroyDescriptorSetLayout(vulkanProgramInfo.renderDevice,
                                     vulkanProgramInfo.descriptorSetLayout,
                                     nullptr);

        vkFreeMemory(vulkanProgramInfo.renderDevice,
                     vulkanProgramInfo.indexBufferMemory,
                     nullptr);
        vkDestroyBuffer(vulkanProgramInfo.renderDevice,
                        vulkanProgramInfo.indexBuffer,
                        nullptr);

        vkFreeMemory(vulkanProgramInfo.renderDevice,
                     vulkanProgramInfo.vertexBufferMemory,
                     nullptr);

        vkDestroyBuffer(vulkanProgramInfo.renderDevice,
                        vulkanProgramInfo.vertexBuffer,
                        nullptr);

        for (std::size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            vkDestroySemaphore(vulkanProgramInfo.renderDevice,
                               vulkanProgramInfo.renderFinishedSemaphores[i],
                               nullptr);

            vkDestroySemaphore(vulkanProgramInfo.renderDevice,
                               vulkanProgramInfo.nextImageReadySemaphores[i],
                               nullptr);

            vkDestroyFence(vulkanProgramInfo.renderDevice,
                           vulkanProgramInfo.frameReadyFences[i],
                           nullptr);
        }

        vkDestroyCommandPool(vulkanProgramInfo.renderDevice,
                             vulkanProgramInfo.commandPool,
                             nullptr);

        for (const VkFramebuffer &swapchainFramebuffer: vulkanProgramInfo.swapchainFramebuffers)
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
            return std::clamp((uint32_t) 3,
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
        std::cerr << failMessage << std::endl;
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

    void createTextureImageView()
    {
        // Create Image of texture image
        VkImageViewCreateInfo textureImageViewCreateInfo{};
        textureImageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        textureImageViewCreateInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
        textureImageViewCreateInfo.image = vulkanProgramInfo.textureImage;
        textureImageViewCreateInfo.subresourceRange =
                {
                        VK_IMAGE_ASPECT_COLOR_BIT,
                        0,
                        1,
                        0,
                        1
                };
        textureImageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;

        vkCreateImageView(vulkanProgramInfo.renderDevice, &textureImageViewCreateInfo, nullptr,
                          &vulkanProgramInfo.textureImageView);

        // Create a image sampler which is actually independent from any image but the tutorial did separate these two
        // as two functions so I will just do the same here
        VkSamplerCreateInfo imageSamplerCreateInfo{};
        imageSamplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        imageSamplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        imageSamplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        imageSamplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
        imageSamplerCreateInfo.anisotropyEnable = VK_TRUE;
        imageSamplerCreateInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        imageSamplerCreateInfo.compareEnable = VK_FALSE;
        imageSamplerCreateInfo.magFilter = VK_FILTER_LINEAR;
        imageSamplerCreateInfo.minFilter = VK_FILTER_LINEAR;
        imageSamplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        imageSamplerCreateInfo.minLod = 0.0f;
        imageSamplerCreateInfo.maxLod = 0.0f;
        imageSamplerCreateInfo.mipLodBias = 0.0f;
        imageSamplerCreateInfo.unnormalizedCoordinates = VK_FALSE;

        VkPhysicalDeviceProperties physicalDeviceProperties{};
        vkGetPhysicalDeviceProperties(vulkanProgramInfo.GPU, &physicalDeviceProperties);

        imageSamplerCreateInfo.maxAnisotropy = physicalDeviceProperties.limits.maxSamplerAnisotropy;

        vkCreateSampler(vulkanProgramInfo.renderDevice,
                        &imageSamplerCreateInfo,
                        nullptr,
                        &vulkanProgramInfo.textureImageSampler);
    }

    void createDepthBuffer()
    {

        // For Depth Buffer format, we simply choose VK_FORMAT_D32_SFLOAT
        VkFormat depthBufferFormat = VK_FORMAT_D32_SFLOAT;

        // Create depth image
        VkImageCreateInfo depthImageCreateInfo{};
        depthImageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        depthImageCreateInfo.format = depthBufferFormat;
        depthImageCreateInfo.pNext = nullptr;
        depthImageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        depthImageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
        depthImageCreateInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        depthImageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depthImageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        depthImageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        depthImageCreateInfo.queueFamilyIndexCount = 0;
        depthImageCreateInfo.extent = {vulkanProgramInfo.swapchainExtent.width,
                                       vulkanProgramInfo.swapchainExtent.height,
                                       1};
        depthImageCreateInfo.arrayLayers = 1;
        depthImageCreateInfo.mipLevels = 1;

        vkCreateImage(vulkanProgramInfo.renderDevice, &depthImageCreateInfo, nullptr, &vulkanProgramInfo.depthImage);

        VkMemoryRequirements depthImageMemoryRequirements;
        vkGetImageMemoryRequirements(vulkanProgramInfo.renderDevice, vulkanProgramInfo.depthImage,
                                     &depthImageMemoryRequirements);

        // Allocate Image memory
        VkMemoryAllocateInfo depthImageAllocateInfo{};
        depthImageAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        depthImageAllocateInfo.allocationSize = depthImageMemoryRequirements.size;

        VkPhysicalDeviceMemoryProperties memoryProperties;
        vkGetPhysicalDeviceMemoryProperties(vulkanProgramInfo.GPU,
                                            &memoryProperties);

        bool foundCorrectMemoryType = false;
        VkMemoryPropertyFlags depthImageMemoryProperty = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        for (int i = 0; i < memoryProperties.memoryTypeCount; i++)
        {
            if (depthImageMemoryRequirements.memoryTypeBits & (1 << i) &&
                depthImageMemoryProperty & memoryProperties.memoryTypes[i].propertyFlags)
            {
                depthImageAllocateInfo.memoryTypeIndex = i;
                foundCorrectMemoryType = true;
            }
        }

        if (!foundCorrectMemoryType)
        {
            throw std::runtime_error("Failed to find suitable memory type for depth buffer");
        }

        vkAllocateMemory(vulkanProgramInfo.renderDevice, &depthImageAllocateInfo,
                         nullptr, &vulkanProgramInfo.depthImageMemory);

        // Bind Image and its corresponding memory together
        vkBindImageMemory(vulkanProgramInfo.renderDevice,
                          vulkanProgramInfo.depthImage,
                          vulkanProgramInfo.depthImageMemory,
                          0);

        // Create Depth Image View
        VkImageViewCreateInfo depthImageViewCreateInfo{};
        depthImageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        depthImageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        depthImageViewCreateInfo.format = VK_FORMAT_D32_SFLOAT;
        depthImageViewCreateInfo.image = vulkanProgramInfo.depthImage;
        depthImageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        depthImageViewCreateInfo.subresourceRange.layerCount = 1;
        depthImageViewCreateInfo.subresourceRange.levelCount = 1;
        depthImageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
        depthImageViewCreateInfo.subresourceRange.baseMipLevel = 0;

        vkCreateImageView(vulkanProgramInfo.renderDevice,
                          &depthImageViewCreateInfo,
                          nullptr,
                          &vulkanProgramInfo.depthImageView);

        // Image created, Image View created and Image Memory allocated and binded
    }

    void loadModel()
    {
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string warn, err;

        if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, mesh_path.c_str()))
        {
            throw std::runtime_error(warn + err);
        }
        for (const auto &shape: shapes)
        {
            for (const auto &index: shape.mesh.indices)
            {
                Vertex vertex{};
                vertex.pos = {
                        attrib.vertices[3 * index.vertex_index + 0],
                        attrib.vertices[3 * index.vertex_index + 1],
                        attrib.vertices[3 * index.vertex_index + 2]
                };

                vertex.texCoord = {
                        attrib.texcoords[2 * index.texcoord_index + 0],
                        attrib.texcoords[2 * index.texcoord_index + 1]
                };

                vertex.color = {1.0f, 1.0f, 1.0f};
                vertex.texCoord = 
                {
                    attrib.texcoords[2 * index.texcoord_index + 0],
                    1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
                };

                vertices.push_back(vertex);
                vertex_indices.push_back(vertex_indices.size());
            }
        }
    }
};


int main()
{
    VulkanProgram program{};
    program.run();
}
