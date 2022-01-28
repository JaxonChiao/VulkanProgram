cmake_minimum_required(VERSION 3.16)
project(VulkanProgram LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


add_executable(VulkanProgram src/main.cpp)


if (${CMAKE_SYSTEM_NAME} STREQUAL Darwin)

	message("Building for macOS")
	
	include_directories(${CMAKE_CURRENT_SOURCE_DIR}/VulkanSDK/macOS/include/vulkan)

	target_link_libraries(VulkanProgram 
		${CMAKE_CURRENT_SOURCE_DIR}/VulkanSDK/macOS/lib/libvulkan.1.2.189.dylib
		${CMAKE_CURRENT_SOURCE_DIR}/VulkanSDK/macOS/lib/libvulkan.1.dylib)
elseif(${CMAKE_SYSTEM_NAME} STREQUAL Linux)

	message("Building for Ubuntu")



endif()
