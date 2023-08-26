#pragma once

#include <vulkan/vulkan.hpp>

#include "core.hpp"
#include "mesh.hpp"
#include "shader.hpp"

namespace hc {

struct PipelineConfig {
    std::vector<vk::UniqueShaderModule> shaders{};
    std::vector<vk::ShaderStageFlagBits> stage_flags{};
    vk::Extent2D extent{};
    vk::PolygonMode mode{vk::PolygonMode::eFill};
};

class PipelineBuilder {
public:
    PipelineBuilder& new_pipeline();

    PipelineBuilder& add_vertex_shader(vk::UniqueShaderModule shader);
    PipelineBuilder& add_fragment_shader(vk::UniqueShaderModule shader);

    PipelineBuilder& set_extent(const vk::Extent2D& extent);

    PipelineBuilder& set_polygon_mode(vk::PolygonMode mode);

    [[nodiscard]]
    std::vector<vk::UniquePipeline> build(
        const vk::Device& device,
        const vk::RenderPass& render_pass
    );

private:
    std::vector<PipelineConfig> _config{};
    std::vector<std::vector<vk::PipelineShaderStageCreateInfo>> _stages{};
    std::vector<vk::PipelineRasterizationStateCreateInfo> _rasterizers{};
    usize _idx{};
};

}  // namespace hc
