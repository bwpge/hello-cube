#pragma once

#include <imgui.h>
#include <glm/glm.hpp>
#include <vulkan/vulkan.hpp>

#include "hvk/buffer.hpp"
#include "hvk/pipeline_builder.hpp"
#include "hvk/texture.hpp"

struct GLFWwindow;

namespace hvk {

struct MouseData {
    bool is_captured{};
    std::array<bool, 3> button_down{};
};

class UI {
public:
    struct PushConstantBlock {
        glm::vec2 scale{};
        glm::vec2 translate{};
    };

    UI() = default;
    explicit UI(const vk::UniqueRenderPass& render_pass);

    void update(const glm::vec2& display, const MouseData& mouse_data);
    void on_resize();
    void on_scroll(double dx, double dy);
    void on_mouse_move(glm::vec2 pos);
    void draw(const vk::UniqueCommandBuffer& cmd);

private:
    void build_pipeline();

    Texture2D _font_tex{};
    vk::RenderPass _render_pass{};
    vk::UniqueDescriptorPool _pool{};
    vk::UniqueDescriptorSetLayout _descriptor_set_layout{};
    vk::DescriptorSet _descriptor_set{};
    GraphicsPipeline _gfx_pipeline{};
    Buffer _vertex{}, _index{};
    PushConstantBlock _push_constants{};
};

}  // namespace hvk
