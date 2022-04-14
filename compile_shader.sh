#!/bin/bash
VulkanSDK/macOS/bin/glslc -o src/spvShaders/vert.spv src/glslShaders/shader.vert
VulkanSDK/macOS/bin/glslc -o src/spvShaders/frag.spv src/glslShaders/shader.frag
