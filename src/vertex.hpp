//
// Created by Jiaxiong Jiao on 2022-04-14.
//
#include <glm/glm.hpp>

#ifndef VULKANPROGRAM_VERTEX_HPP
#define VULKANPROGRAM_VERTEX_HPP
struct Vertex
{
    glm::vec2 pos;
    glm::vec3 color;
};

const std::vector<struct Vertex> vertices =
        {
                {{0.0f,  -0.5f}, {1.0f, 0.0f, 0.0f}},
                {{0.5f,  0.5f},  {0.0f, 1.0f, 0.0f}},
                {{-0.5f, 0.5f},  {0.0f, 0.0f, 1.0f}}
        };

Vertex vertices_array[] =
        {
                {{0.0f,  -0.5f}, {1.0f, 0.0f, 0.0f}},
                {{0.5f,  0.5f},  {0.0f, 1.0f, 0.0f}},
                {{-0.5f, 0.5f},  {0.0f, 0.0f, 1.0f}}
        };

#endif //VULKANPROGRAM_VERTEX_HPP
