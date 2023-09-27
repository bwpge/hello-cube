#pragma once

#include <vulkan/vulkan.hpp>

#include "hvk/core.hpp"
#include "hvk/shader.hpp"

namespace hvk {

struct GraphicsPipeline {
    vk::UniquePipelineLayout layout;
    std::vector<vk::UniquePipeline> pipelines;
};

struct PipelineConfig {
    std::vector<vk::UniqueShaderModule> shaders{};
    std::vector<vk::ShaderStageFlagBits> stage_flags{};
    std::vector<vk::VertexInputBindingDescription> vertex_input_bindings{};
    std::vector<vk::VertexInputAttributeDescription> vertex_input_attrs{};
    vk::PipelineInputAssemblyStateCreateInfo input_assembly_state{
        {},
        vk::PrimitiveTopology::eTriangleList,
        VK_FALSE,
    };
    // vk::Extent2D extent{};
    std::vector<vk::Viewport> viewports{};
    std::vector<vk::Rect2D> scissors{};
    vk::PipelineMultisampleStateCreateInfo multisample_state{};
    std::vector<vk::PipelineColorBlendAttachmentState> color_blend_attachments{};
    vk::PipelineRasterizationStateCreateInfo rasterizer_info{};
    vk::PipelineDepthStencilStateCreateInfo depth_stencil{};
    std::vector<vk::DynamicState> dynamic_states{};
};

class PipelineBuilder {
public:
    PipelineBuilder& new_pipeline();

    PipelineBuilder& add_push_constant(const vk::PushConstantRange& range);
    PipelineBuilder& add_descriptor_set_layout(const vk::UniqueDescriptorSetLayout& layout);
    PipelineBuilder& add_vertex_shader(vk::UniqueShaderModule shader);
    PipelineBuilder& add_vertex_shader(const Shader& shader);
    PipelineBuilder& add_fragment_shader(vk::UniqueShaderModule shader);
    PipelineBuilder& add_fragment_shader(const Shader& shader);
    PipelineBuilder& add_vertex_binding_description(const vk::VertexInputBindingDescription& desc);
    PipelineBuilder& add_vertex_binding_description(
        const std::vector<vk::VertexInputBindingDescription>& desc
    );
    PipelineBuilder& add_vertex_attr_description(const vk::VertexInputAttributeDescription& desc);
    PipelineBuilder& add_vertex_attr_description(
        const std::vector<vk::VertexInputAttributeDescription>& desc
    );
    PipelineBuilder& with_input_assembly_state(const vk::PipelineInputAssemblyStateCreateInfo& info
    );
    PipelineBuilder& with_flipped_viewport(const vk::Extent2D& extent);
    PipelineBuilder& with_viewport(const vk::Extent2D& extent);
    PipelineBuilder& add_dynamic_state(vk::DynamicState state);
    PipelineBuilder& with_multisample_state(const vk::PipelineMultisampleStateCreateInfo& info);
    PipelineBuilder& with_default_color_blend_opaque();
    PipelineBuilder& with_default_color_blend_transparency();
    PipelineBuilder& with_front_face(vk::FrontFace front);
    PipelineBuilder& with_cull_mode(vk::CullModeFlagBits mode);
    PipelineBuilder& with_polygon_mode(vk::PolygonMode mode);
    PipelineBuilder& with_depth_stencil(bool test, bool write, vk::CompareOp op);

    [[nodiscard]]
    GraphicsPipeline build(const vk::RenderPass& render_pass);

private:
    [[nodiscard]]
    PipelineConfig& current_config();
    [[nodiscard]]
    vk::UniquePipelineLayout create_pipeline_layout() const;

    usize _idx{};
    std::vector<PipelineConfig> _config{};
    std::vector<vk::PushConstantRange> _push_constants{};
    std::vector<vk::DescriptorSetLayout> _desc_set_layouts{};
};

}  // namespace hvk
