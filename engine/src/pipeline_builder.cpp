#include "pipeline_builder.hpp"

namespace hc {

std::vector<vk::PipelineShaderStageCreateInfo> build_shader_stage_info(
    const PipelineConfig& config
) {
    HC_ASSERT(
        config.stage_flags.size() == config.shaders.size(),
        "number of stage_flags should always equal number of shaders"
    );
    std::vector<vk::PipelineShaderStageCreateInfo> stages{};

    auto stage_count = config.shaders.size();
    for (usize i = 0; i < stage_count; i++) {
        vk::PipelineShaderStageCreateInfo stage{
            {},
            config.stage_flags[i],
            config.shaders[i].get(),
            "main",
        };
        stages.push_back(stage);
    }

    return stages;
}

PipelineBuilder& PipelineBuilder::new_pipeline() {
    _config.push_back(PipelineConfig{});
    _idx = _config.size() - 1;
    return *this;
}

PipelineBuilder& PipelineBuilder::add_fragment_shader(
    vk::UniqueShaderModule shader
) {
    _config[_idx].shaders.push_back(std::move(shader));
    _config[_idx].stage_flags.push_back(vk::ShaderStageFlagBits::eFragment);
    return *this;
}

PipelineBuilder& PipelineBuilder::add_vertex_shader(
    vk::UniqueShaderModule shader
) {
    _config[_idx].shaders.push_back(std::move(shader));
    _config[_idx].stage_flags.push_back(vk::ShaderStageFlagBits::eVertex);
    return *this;
}

PipelineBuilder& PipelineBuilder::set_extent(const vk::Extent2D& extent) {
    _config[_idx].extent = extent;
    return *this;
}

PipelineBuilder& PipelineBuilder::set_front_face(vk::FrontFace front) {
    _config[_idx].front_face = front;
    return *this;
}

PipelineBuilder& PipelineBuilder::set_cull_mode(vk::CullModeFlagBits mode) {
    _config[_idx].cull_mode = mode;
    return *this;
}

PipelineBuilder& PipelineBuilder::set_polygon_mode(vk::PolygonMode mode) {
    _config[_idx].polygon_mode = mode;
    return *this;
}

PipelineBuilder& PipelineBuilder::set_push_constant(
    vk::PushConstantRange push_constant
) {
    _push_constant = push_constant;
    return *this;
}

[[nodiscard]]
GraphicsPipeline PipelineBuilder::build(
    const vk::Device& device,
    const vk::RenderPass& render_pass
) {
    auto vertex_binding = Vertex::get_binding_description();
    auto vertex_attr = Vertex::get_attr_description();
    vk::PipelineVertexInputStateCreateInfo vertex_input{};
    vertex_input.setVertexBindingDescriptions(vertex_binding);
    vertex_input.setVertexAttributeDescriptions(vertex_attr);

    vk::PipelineInputAssemblyStateCreateInfo input_assembly{
        {},
        vk::PrimitiveTopology::eTriangleList,
        VK_FALSE,
    };
    vk::Viewport viewport{
        0.0f,
        0.0f,
        static_cast<float>(_config[_idx].extent.width),
        static_cast<float>(_config[_idx].extent.height),
        0.0f,
        1.0f,
    };
    vk::Rect2D scissor{{0, 0}, _config[_idx].extent};
    vk::PipelineViewportStateCreateInfo viewport_info{};
    viewport_info.setScissors(scissor).setViewports(viewport);

    vk::PipelineMultisampleStateCreateInfo multisampling{};
    multisampling.setMinSampleShading(1.0f);

    vk::PipelineColorBlendAttachmentState color_blend_attachment{};
    color_blend_attachment.setColorWriteMask(
        vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
        vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA
    );

    vk::PipelineColorBlendStateCreateInfo color_blend{};
    color_blend.setAttachments(color_blend_attachment);

    vk::PipelineLayoutCreateInfo layout_info{};
    if (_push_constant.size > 0) {
        layout_info.setPushConstantRanges(_push_constant);
    }
    auto layout = device.createPipelineLayoutUnique(layout_info);

    std::vector<vk::GraphicsPipelineCreateInfo> pipeline_infos{};
    _rasterizers.resize(_config.size(), {});
    for (usize i = 0; i < _config.size(); i++) {
        std::vector<vk::PipelineShaderStageCreateInfo> stages{};

        build_shader_stage_info(_config[i]);
        auto stage_count = _config[i].shaders.size();
        for (usize j = 0; j < stage_count; j++) {
            vk::PipelineShaderStageCreateInfo stage{
                {},
                _config[i].stage_flags[j],
                _config[i].shaders[j].get(),
                "main",
            };
            stages.push_back(stage);
        }
        _stages.push_back(stages);

        _rasterizers[i]
            .setLineWidth(1.0f)
            .setCullMode(_config[i].cull_mode)
            .setFrontFace(_config[i].front_face)
            .setPolygonMode(_config[i].polygon_mode);

        vk::GraphicsPipelineCreateInfo info{};
        info.setStages(_stages[i])
            .setPVertexInputState(&vertex_input)
            .setPInputAssemblyState(&input_assembly)
            .setPViewportState(&viewport_info)
            .setPRasterizationState(&_rasterizers[i])
            .setPMultisampleState(&multisampling)
            .setPColorBlendState(&color_blend)
            .setLayout(layout.get())
            .setRenderPass(render_pass);
        pipeline_infos.push_back(info);
    }

    auto pipelines =
        device.createGraphicsPipelinesUnique(nullptr, pipeline_infos);
    if (pipelines.result != vk::Result::eSuccess) {
        PANIC("Failed to create graphics pipeline");
    }

    _config.clear();
    _stages.clear();
    _rasterizers.clear();
    _idx = 0;

    GraphicsPipeline result{std::move(layout), std::move(pipelines.value)};
    return result;
}

}  // namespace hc
