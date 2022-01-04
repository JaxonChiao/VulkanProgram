cmake_minimum_required(VERSION 3.16)
project(VulkanProgram LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/VulkanSDK/x86_64/include/vulkan)

add_executable(VulkanProgram src/main.cpp)

target_link_libraries(VulkanProgram 
	${CMAKE_CURRENT_SOURCE_DIR}/VulkanSDK/x86_64/lib/libvulkan.so.1.2.198
	${CMAKE_CURRENT_SOURCE_DIR}/VulkanSDK/x86_64/lib/libvulkan.so.1
)

