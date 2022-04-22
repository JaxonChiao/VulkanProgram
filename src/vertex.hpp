//
// Created by Jiaxiong Jiao on 2022-04-14.
//
#include <glm/glm.hpp>
#include <vector>

#ifndef VULKANPROGRAM_VERTEX_HPP
#define VULKANPROGRAM_VERTEX_HPP
struct Vertex
{
    glm::vec2 pos;
    glm::vec3 color;
};

const std::vector<struct Vertex> vertices =
        {
                {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
                {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
                {{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
                {{-0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
        };

const std::vector<uint32_t> vertex_indices =
        {
                0, 1, 2, 0, 2, 3
        };


#endif //VULKANPROGRAM_VERTEX_HPP
