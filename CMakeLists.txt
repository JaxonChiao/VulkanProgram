cmake_minimum_required(VERSION 3.16)
project(VulkanProgram LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Turn off irrelevant GLFW builds
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

add_executable(VulkanProgram src/main.cpp)

# Add glfw library
add_subdirectory(glfw)
target_link_libraries(VulkanProgram glfw)

# Add glm include directory
include_directories(glm)

# Add stb_img include directory
target_include_directories(VulkanProgram
    PRIVATE stb)

# Add tinyobjloader
target_include_directories(VulkanProgram
    PRIVATE tinyobjloader
)

if (${CMAKE_SYSTEM_NAME} STREQUAL Darwin)

	message("Building for macOS")
	
	include_directories(${CMAKE_CURRENT_SOURCE_DIR}/VulkanSDK/macOS/include)

	target_link_libraries(VulkanProgram 
        ${CMAKE_CURRENT_SOURCE_DIR}/VulkanSDK/macOS/lib/libvulkan.1.2.189.dylib
        ${CMAKE_CURRENT_SOURCE_DIR}/VulkanSDK/macOS/lib/libvulkan.1.dylib)

elseif(${CMAKE_SYSTEM_NAME} STREQUAL Linux)

	message("Building for Ubuntu")

    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/VulkanSDK/x86_64/include)

    target_link_libraries(VulkanProgram
        ${CMAKE_CURRENT_SOURCE_DIR}/VulkanSDK/x86_64/lib/libvulkan.so.1
        ${CMAKE_CURRENT_SOURCE_DIR}/VulkanSDK/x86_64/lib/libvulkan.so.1.2.198)



endif()
