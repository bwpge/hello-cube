#pragma once

#include "glm/glm.hpp"

namespace hc {

struct Vertex {
    glm::vec3 pos;
    glm::vec3 color;

    static std::vector<vk::VertexInputBindingDescription>
    get_binding_description() {
        return {{
            0,
            sizeof(Vertex),
            vk::VertexInputRate::eVertex,
        }};
    }

    static std::vector<vk::VertexInputAttributeDescription>
    get_attr_description() {
        auto pos_attr = vk::VertexInputAttributeDescription{
            0,
            0,
            vk::Format::eR32G32B32Sfloat,
            offsetof(Vertex, pos),
        };
        auto color_attr = vk::VertexInputAttributeDescription{
            1,
            0,
            vk::Format::eR32G32B32Sfloat,
            offsetof(Vertex, color),
        };

        return {
            pos_attr,
            color_attr,
        };
    }
};

}  // namespace hc
