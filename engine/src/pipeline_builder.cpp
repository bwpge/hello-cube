#include "hvk/pipeline_builder.hpp"
#include "hvk/vk_context.hpp"

namespace hvk {

struct PipelineBuilderState {
    explicit PipelineBuilderState(usize count)
        : pipeline_infos(count),
          vertex_input_states(count),
          viewport_states(count),
          shader_stages(count),
          rasterizers(count),
          color_blend_states(count) {}

    // NOLINTBEGIN(misc-non-private-member-variables-in-classes)
    std::vector<vk::GraphicsPipelineCreateInfo> pipeline_infos{};
    std::vector<vk::PipelineVertexInputStateCreateInfo> vertex_input_states{};
    std::vector<vk::PipelineViewportStateCreateInfo> viewport_states{};
    std::vector<std::vector<vk::PipelineShaderStageCreateInfo>> shader_stages{};
    std::vector<vk::PipelineRasterizationStateCreateInfo> rasterizers{};
    std::vector<vk::PipelineColorBlendStateCreateInfo> color_blend_states{};
    // NOLINTEND(misc-non-private-member-variables-in-classes)
};

std::vector<vk::PipelineShaderStageCreateInfo> build_shader_stage_info(const PipelineConfig& config
) {
    HVK_ASSERT(
        config.stage_flags.size() == config.shaders.size(),
        "number of stage_flags should always equal number of shaders"
    );

    std::vector<vk::PipelineShaderStageCreateInfo> stages{};
    for (usize i = 0; i < config.shaders.size(); i++) {
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

vk::PipelineColorBlendAttachmentState default_color_blend_attachment(bool blend) {
    vk::PipelineColorBlendAttachmentState state{};
    state
        .setColorWriteMask(
            vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG
            | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA
        )
        .setBlendEnable(VK_FALSE)
        .setSrcColorBlendFactor(vk::BlendFactor::eOne)
        .setDstColorBlendFactor(vk::BlendFactor::eZero)
        .setColorBlendOp(vk::BlendOp::eAdd)
        .setSrcAlphaBlendFactor(vk::BlendFactor::eOne)
        .setDstAlphaBlendFactor(vk::BlendFactor::eZero)
        .setAlphaBlendOp(vk::BlendOp::eAdd);

    if (blend) {
        state.setBlendEnable(VK_TRUE)
            .setSrcColorBlendFactor(vk::BlendFactor::eSrcAlpha)
            .setDstColorBlendFactor(vk::BlendFactor::eOneMinusSrcAlpha);
    }
    return state;
}

PipelineBuilder& PipelineBuilder::new_pipeline() {
    _config.push_back(PipelineConfig{});
    _idx = _config.size() - 1;

    auto& config = current_config();
    config.rasterizer_info.setCullMode(vk::CullModeFlagBits::eBack).setLineWidth(1.0f);
    config.color_blend_attachments = {default_color_blend_attachment(false)};

    return *this;
}

PipelineBuilder& PipelineBuilder::add_push_constant(const vk::PushConstantRange& range) {
    _push_constants.push_back(range);
    return *this;
}

PipelineBuilder& PipelineBuilder::add_descriptor_set_layout(
    const vk::UniqueDescriptorSetLayout& layout
) {
    _desc_set_layouts.push_back(layout.get());
    return *this;
}

PipelineBuilder& PipelineBuilder::add_vertex_shader(vk::UniqueShaderModule shader) {
    current_config().shaders.push_back(std::move(shader));
    current_config().stage_flags.push_back(vk::ShaderStageFlagBits::eVertex);
    return *this;
}

PipelineBuilder& PipelineBuilder::add_vertex_shader(const Shader& shader) {
    add_vertex_shader(shader.module());
    return *this;
}

PipelineBuilder& PipelineBuilder::add_fragment_shader(vk::UniqueShaderModule shader) {
    current_config().shaders.push_back(std::move(shader));
    current_config().stage_flags.push_back(vk::ShaderStageFlagBits::eFragment);
    return *this;
}

PipelineBuilder& PipelineBuilder::add_fragment_shader(const Shader& shader) {
    add_fragment_shader(shader.module());
    return *this;
}

PipelineBuilder& PipelineBuilder::add_vertex_binding_description(
    const vk::VertexInputBindingDescription& desc
) {
    current_config().vertex_input_bindings.push_back(desc);
    return *this;
}

PipelineBuilder& PipelineBuilder::add_vertex_binding_description(
    const std::vector<vk::VertexInputBindingDescription>& desc
) {
    for (const auto& d : desc) {
        current_config().vertex_input_bindings.push_back(d);
    }
    return *this;
}

PipelineBuilder& PipelineBuilder::add_vertex_attr_description(
    const vk::VertexInputAttributeDescription& desc
) {
    current_config().vertex_input_attrs.push_back(desc);
    return *this;
}

PipelineBuilder& PipelineBuilder::add_vertex_attr_description(
    const std::vector<vk::VertexInputAttributeDescription>& desc
) {
    for (const auto& d : desc) {
        current_config().vertex_input_attrs.push_back(d);
    }
    return *this;
}

PipelineBuilder& PipelineBuilder::with_input_assembly_state(
    const vk::PipelineInputAssemblyStateCreateInfo& info
) {
    current_config().input_assembly_state = info;
    return *this;
}

PipelineBuilder& PipelineBuilder::with_default_viewport(const vk::Extent2D& extent) {
    auto& config = current_config();
    config.viewports = {vk::Viewport{
        0.0f,
        static_cast<float>(extent.height),
        static_cast<float>(extent.width),
        -static_cast<float>(extent.height),
        0.0f,
        1.0f,
    }};
    config.scissors = {vk::Rect2D{{0, 0}, extent}};
    return *this;
}

PipelineBuilder& PipelineBuilder::with_multisample_state(
    const vk::PipelineMultisampleStateCreateInfo& info
) {
    current_config().multisample_state = info;
    return *this;
}

PipelineBuilder& PipelineBuilder::with_default_color_blend_opaque() {
    current_config().color_blend_attachments = {default_color_blend_attachment(false)};
    return *this;
}

PipelineBuilder& PipelineBuilder::with_default_color_blend_transparency() {
    current_config().color_blend_attachments = {default_color_blend_attachment(true)};
    return *this;
}

PipelineBuilder& PipelineBuilder::with_front_face(vk::FrontFace front) {
    current_config().rasterizer_info.setFrontFace(front);
    return *this;
}

PipelineBuilder& PipelineBuilder::with_cull_mode(vk::CullModeFlagBits mode) {
    current_config().rasterizer_info.setCullMode(mode);
    return *this;
}

PipelineBuilder& PipelineBuilder::with_polygon_mode(vk::PolygonMode mode) {
    current_config().rasterizer_info.setPolygonMode(mode);
    return *this;
}

PipelineBuilder& PipelineBuilder::with_depth_stencil(bool test, bool write, vk::CompareOp op) {
    vk::PipelineDepthStencilStateCreateInfo info = {};
    info.setDepthTestEnable(test ? VK_TRUE : VK_FALSE)
        .setDepthWriteEnable(write ? VK_TRUE : VK_FALSE)
        .setDepthCompareOp(test ? op : vk::CompareOp::eAlways)
        .setDepthBoundsTestEnable(VK_FALSE)
        .setStencilTestEnable(VK_FALSE);

    current_config().depth_stencil = info;
    return *this;
}

[[nodiscard]]
GraphicsPipeline PipelineBuilder::build(const vk::RenderPass& render_pass) {
    const auto count = _config.size();
    auto layout = create_pipeline_layout();
    PipelineBuilderState state{count};

    for (u32 i = 0; i < count; i++) {
        const auto& config = _config[i];

        state.vertex_input_states[i]
            .setVertexBindingDescriptions(config.vertex_input_bindings)
            .setVertexAttributeDescriptions(config.vertex_input_attrs);
        state.viewport_states[i].setViewports(config.viewports).setScissors(config.scissors);
        state.shader_stages[i] = build_shader_stage_info(_config[i]);
        state.color_blend_states[i].setAttachments(config.color_blend_attachments);

        state.pipeline_infos[i]
            .setStages(state.shader_stages[i])
            .setPVertexInputState(&state.vertex_input_states[i])
            .setPInputAssemblyState(&config.input_assembly_state)
            .setPViewportState(&state.viewport_states[i])
            .setPRasterizationState(&config.rasterizer_info)
            .setPMultisampleState(&config.multisample_state)
            .setPColorBlendState(&state.color_blend_states[i])
            .setPDepthStencilState(&config.depth_stencil)
            .setLayout(layout.get())
            .setRenderPass(render_pass);
    }

    auto pipelines =
        VulkanContext::device().createGraphicsPipelinesUnique(nullptr, state.pipeline_infos);
    VKHPP_CHECK(pipelines.result, "Failed to create graphics pipeline");
    GraphicsPipeline result{std::move(layout), std::move(pipelines.value)};

    *this = {};

    return result;
}

PipelineConfig& PipelineBuilder::current_config() {
    return _config[_idx];
}

vk::UniquePipelineLayout PipelineBuilder::create_pipeline_layout() const {
    vk::PipelineLayoutCreateInfo layout_info{};
    if (!_desc_set_layouts.empty()) {
        layout_info.setSetLayouts(_desc_set_layouts);
    }
    if (!_push_constants.empty()) {
        layout_info.setPushConstantRanges(_push_constants);
    }

    return VulkanContext::device().createPipelineLayoutUnique(layout_info);
}

}  // namespace hvk
