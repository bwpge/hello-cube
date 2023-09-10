#pragma once

#include <vulkan/vulkan.hpp>

#include "hvk/core.hpp"
#include "hvk/mesh.hpp"
#include "hvk/shader.hpp"

namespace hvk {

struct GraphicsPipeline {
    vk::UniquePipelineLayout layout;
    std::vector<vk::UniquePipeline> pipelines;
};

struct PipelineConfig {
    std::vector<vk::UniqueShaderModule> shaders{};
    std::vector<vk::ShaderStageFlagBits> stage_flags{};
    vk::CullModeFlagBits cull_mode{vk::CullModeFlagBits::eBack};
    vk::Extent2D extent{};
    vk::PolygonMode polygon_mode{vk::PolygonMode::eFill};
    vk::FrontFace front_face{vk::FrontFace::eCounterClockwise};
    vk::PipelineDepthStencilStateCreateInfo depth_stencil{};
};

class PipelineBuilder {
public:
    PipelineBuilder& new_pipeline();

    PipelineBuilder& add_vertex_shader(vk::UniqueShaderModule shader);
    PipelineBuilder& add_fragment_shader(vk::UniqueShaderModule shader);
    PipelineBuilder& add_descriptor_set_layout(
        const vk::UniqueDescriptorSetLayout& layout
    );
    PipelineBuilder& set_extent(const vk::Extent2D& extent);
    PipelineBuilder& set_front_face(vk::FrontFace front);
    PipelineBuilder& set_cull_mode(vk::CullModeFlagBits mode);
    PipelineBuilder& set_polygon_mode(vk::PolygonMode mode);
    PipelineBuilder& set_push_constant(vk::PushConstantRange push_constant);
    PipelineBuilder& set_depth_stencil(bool test, bool write, vk::CompareOp op);

    [[nodiscard]]
    GraphicsPipeline build(
        const vk::Device& device,
        const vk::RenderPass& render_pass
    );

private:
    usize _idx{};
    std::vector<PipelineConfig> _config{};
    std::vector<std::vector<vk::PipelineShaderStageCreateInfo>> _stages{};
    std::vector<vk::PipelineRasterizationStateCreateInfo> _rasterizers{};
    vk::PushConstantRange _push_constant{};
    std::vector<vk::DescriptorSetLayout> _desc_set_layouts{};
};

}  // namespace hvk
