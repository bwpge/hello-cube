#pragma once

#include "glm/glm.hpp"

#include "hvk/core.hpp"
#include "hvk/texture.hpp"

namespace hvk {

struct Material {
    float alpha_cutoff{1.0f};
    float metallic_factor{1.0f};
    float roughness_factor{1.0f};
    glm::vec4 base_color_factor{1.0f};
    glm::vec4 emissive_factor{1.0f};
    Texture* base_color_texture{};
    Texture* metallic_roughness_texture{};
    Texture* normal_texture{};
    Texture* occlusion_texture{};
    Texture* emissive_texture{};
    bool double_sided{false};

    struct TexCoordSets {
        u8 base_color = 0;
        u8 metallic_roughness = 0;
        u8 specular_glossiness = 0;
        u8 normal = 0;
        u8 occlusion = 0;
        u8 emissive = 0;
    } tex_coord_sets{};

    vk::DescriptorSet descriptor_set{};

    static Material none() {
        return {.base_color_factor{0.8f, 0.8f, 0.8f, 1.0f}};
    }
};

}  // namespace hvk
