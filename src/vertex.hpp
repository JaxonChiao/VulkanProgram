//
// Created by Jiaxiong Jiao on 2022-04-14.
//
#include <glm/glm.hpp>
#include <vector>

#ifndef VULKANPROGRAM_VERTEX_HPP
#define VULKANPROGRAM_VERTEX_HPP

struct Vertex
{
    glm::vec3 pos;
    glm::vec3 color;
    glm::vec2 texCoord;
};

struct UniformBufferObject {
    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
};

#endif //VULKANPROGRAM_VERTEX_HPP
