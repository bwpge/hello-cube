#include "hvk/ui.hpp"
#include "hvk/core.hpp"
#include "hvk/descriptor_utils.hpp"
#include "hvk/resource_manager.hpp"

namespace hvk {

UI::UI(const vk::UniqueRenderPass& render_pass) : _render_pass(render_pass.get()) {
    spdlog::trace("Creating UI overlay");
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    // don't save UI state
    ImGui::GetIO().IniFilename = nullptr;

    // load font texture
    auto& io = ImGui::GetIO();
    u8* font_data{};
    i32 font_tex_w{};
    i32 font_tex_h{};
    usize font_tex_size{};

    io.Fonts->AddFontFromFileTTF("../assets/fonts/Inter-Regular.ttf", 16.0f);
    io.Fonts->GetTexDataAsRGBA32(&font_data, &font_tex_w, &font_tex_h);
    font_tex_size = static_cast<usize>(font_tex_w) * static_cast<usize>(font_tex_h) * 4
        * sizeof(u8);
    _font_tex = Texture2D::
        from_buffer(font_data, font_tex_size, font_tex_w, font_tex_h, vk::Format::eR8G8B8A8Unorm);

    // imgui colors
    auto theme_blue = ImVec4{0.0f, 0.478f, 1.0f, 1.0f};

    auto& style = ImGui::GetStyle();
    style.Colors[ImGuiCol_Text] = ImVec4(0.882f, 0.882f, 0.882f, 1.0f);
    style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.882f, 0.882f, 0.882f, 0.50f);
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.078f, 0.082f, 0.086f, 1.0f);
    style.Colors[ImGuiCol_ChildBg] = ImVec4(0.169f, 0.173f, 0.184f, 0.0f);
    style.Colors[ImGuiCol_PopupBg] = ImVec4(0.078f, 0.082f, 0.086f, 1.0f);
    style.Colors[ImGuiCol_Border] = ImVec4(0.227f, 0.227f, 0.239f, 0.29f);
    style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.169f, 0.173f, 0.184f, 0.24f);
    style.Colors[ImGuiCol_FrameBg] = ImVec4(0.102f, 0.106f, 0.11f, 1.0f);
    style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.19f, 0.19f, 0.19f, 0.54f);
    style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.247f, 0.251f, 0.271f, 1.0f);
    style.Colors[ImGuiCol_TitleBg] = ImVec4(0.078f, 0.082f, 0.086f, 1.0f);
    style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.169f, 0.173f, 0.184f, 1.0f);
    style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.078f, 0.082f, 0.086f, 1.0f);
    style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.078f, 0.082f, 0.086f, 1.0f);
    style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.078f, 0.082f, 0.086f, 1.0f);
    style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.34f, 0.34f, 0.34f, 0.54f);
    style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.40f, 0.40f, 0.40f, 0.54f);
    style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.56f, 0.56f, 0.56f, 0.54f);
    style.Colors[ImGuiCol_CheckMark] = theme_blue;
    style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.34f, 0.34f, 0.34f, 0.54f);
    style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.56f, 0.56f, 0.56f, 0.54f);
    style.Colors[ImGuiCol_Button] = ImVec4(0.243f, 0.243f, 0.255f, 1.0f);
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.293f, 0.293f, 0.305f, 1.0f);
    style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.247f, 0.251f, 0.271f, 1.0f);
    style.Colors[ImGuiCol_Header] = ImVec4(0.169f, 0.173f, 0.184f, 0.52f);
    style.Colors[ImGuiCol_HeaderHovered] = theme_blue;
    style.Colors[ImGuiCol_HeaderActive] = theme_blue;
    style.Colors[ImGuiCol_Separator] = ImVec4(0.28f, 0.28f, 0.28f, 0.29f);
    style.Colors[ImGuiCol_SeparatorHovered] = ImVec4(0.44f, 0.44f, 0.44f, 0.29f);
    style.Colors[ImGuiCol_SeparatorActive] = ImVec4(0.40f, 0.44f, 0.47f, 1.0f);
    style.Colors[ImGuiCol_ResizeGrip] = ImVec4(0.169f, 0.173f, 0.184f, 0.54f);
    style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.247f, 0.251f, 0.271f, 0.54f);
    style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.247f, 0.251f, 0.271f, 1.0f);
    style.Colors[ImGuiCol_Tab] = ImVec4(0.169f, 0.173f, 0.184f, 0.25f);
    style.Colors[ImGuiCol_TabHovered] = ImVec4(0.247f, 0.251f, 0.271f, 1.0f);
    style.Colors[ImGuiCol_TabActive] = ImVec4(0.169f, 0.173f, 0.184f, 1.0f);
    style.Colors[ImGuiCol_TabUnfocused] = ImVec4(0.169f, 0.173f, 0.184f, 0.52f);
    style.Colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.14f, 0.14f, 0.14f, 1.0f);
    style.Colors[ImGuiCol_PlotLines] = theme_blue;
    style.Colors[ImGuiCol_PlotLinesHovered] = theme_blue;
    style.Colors[ImGuiCol_PlotHistogram] = theme_blue;
    style.Colors[ImGuiCol_PlotHistogramHovered] = theme_blue;
    style.Colors[ImGuiCol_TableHeaderBg] = ImVec4(0.169f, 0.173f, 0.184f, 0.52f);
    style.Colors[ImGuiCol_TableBorderStrong] = ImVec4(0.169f, 0.173f, 0.184f, 0.52f);
    style.Colors[ImGuiCol_TableBorderLight] = ImVec4(0.28f, 0.28f, 0.28f, 0.29f);
    style.Colors[ImGuiCol_TableRowBg] = ImVec4(0.169f, 0.173f, 0.184f, 0.0f);
    style.Colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.0f, 1.0f, 1.0f, 0.06f);
    style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.247f, 0.251f, 0.271f, 1.0f);
    style.Colors[ImGuiCol_DragDropTarget] = ImVec4(0.33f, 0.67f, 0.86f, 1.0f);
    style.Colors[ImGuiCol_NavHighlight] = theme_blue;
    style.Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.0f, 0.0f, 0.0f, 0.70f);
    style.Colors[ImGuiCol_NavWindowingDimBg] = ImVec4(1.0f, 0.0f, 0.0f, 0.20f);
    style.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(1.0f, 0.0f, 0.0f, 0.35f);

    // imgui style
    style.WindowPadding = ImVec2(8.0f, 8.0f);
    style.FramePadding = ImVec2(5.0f, 2.0f);
    style.CellPadding = ImVec2(6.0f, 6.0f);
    style.ItemSpacing = ImVec2(6.0f, 6.0f);
    style.ItemInnerSpacing = ImVec2(6.0f, 6.0f);
    style.TouchExtraPadding = ImVec2(0.0f, 0.0f);
    style.IndentSpacing = 25.0f;
    style.ScrollbarSize = 15.0f;
    style.GrabMinSize = 10.0f;
    style.WindowBorderSize = 1.0f;
    style.ChildBorderSize = 1.0f;
    style.PopupBorderSize = 1.0f;
    style.FrameBorderSize = 1.0f;
    style.TabBorderSize = 1.0f;
    style.WindowRounding = 7.0f;
    style.ChildRounding = 4.0f;
    style.FrameRounding = 3.0f;
    style.PopupRounding = 4.0f;
    style.ScrollbarRounding = 9.0f;
    style.GrabRounding = 3.0f;
    style.LogSliderDeadzone = 4.0f;
    style.TabRounding = 4.0f;

    // descriptor pool
    std::vector<vk::DescriptorPoolSize> sizes = {
        {vk::DescriptorType::eCombinedImageSampler, 1},
    };
    vk::DescriptorPoolCreateInfo pool_ci{};
    pool_ci.setPoolSizes(sizes).setMaxSets(1);
    _pool = VulkanContext::device().createDescriptorPoolUnique(pool_ci);

    // descriptor layout and set
    DescriptorSetBindingMap map{
        {vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment},
    };
    _descriptor_set_layout = map.build_layout();
    _descriptor_set = VulkanContext::allocate_descriptor_set(_pool, _descriptor_set_layout);

    DescriptorSetWriter writer{};
    writer.write_images(_descriptor_set, map, _font_tex.descriptor_info());

    build_pipeline();
}

void UI::update(const glm::vec2& display, const MouseData& mouse_data) {
    auto& io = ImGui::GetIO();
    if (!mouse_data.is_captured) {
        io.MouseDown[0] = mouse_data.button_down[0];
        io.MouseDown[1] = mouse_data.button_down[1];
        io.MouseDown[2] = mouse_data.button_down[2];
    }

    io.DisplaySize.x = display.x;
    io.DisplaySize.y = display.y;
    _push_constants.scale = {2.0f / display.x, 2.0f / display.y};

    ImGui::NewFrame();
    ImGui::ShowDemoWindow();
    ImGui::Render();

    auto* draw_data = ImGui::GetDrawData();
    if (!draw_data) {
        return;
    }

    _push_constants.translate = {
        -1.0f - draw_data->DisplayPos.x * _push_constants.scale.x,
        -1.0f - draw_data->DisplayPos.y * _push_constants.scale.y
    };

    usize vtx_size = static_cast<usize>(draw_data->TotalVtxCount) * sizeof(ImDrawVert);
    usize idx_size = static_cast<usize>(draw_data->TotalIdxCount) * sizeof(ImDrawIdx);
    if (!vtx_size || !idx_size) {
        return;
    }

    auto update_buffers = !_vertex.buffer() || !_index.buffer() || (_vertex.size() != vtx_size)
        || (_index.size() != idx_size);
    if (update_buffers) {
        VulkanContext::graphics_queue().waitIdle();
        VulkanContext::transfer_queue().waitIdle();
        _vertex = Buffer{vtx_size, vk::BufferUsageFlagBits::eVertexBuffer};
        _index = Buffer{idx_size, vk::BufferUsageFlagBits::eIndexBuffer};
    }

    usize vtx_offset{};
    usize idx_offset{};
    for (i32 i = 0; i < draw_data->CmdListsCount; i++) {
        const auto* cmd_list = draw_data->CmdLists[i];
        _vertex.update(
            cmd_list->VtxBuffer.Data,
            cmd_list->VtxBuffer.Size * sizeof(ImDrawVert),
            vtx_offset
        );
        _index.update(
            cmd_list->IdxBuffer.Data,
            cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx),
            idx_offset
        );
        vtx_offset += static_cast<usize>(cmd_list->VtxBuffer.Size) * sizeof(ImDrawVert);
        idx_offset += static_cast<usize>(cmd_list->IdxBuffer.Size) * sizeof(ImDrawIdx);
    }
}

void UI::on_resize() {
    build_pipeline();
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
void UI::on_scroll(double dx, double dy) {
    auto& io = ImGui::GetIO();
    if (io.WantCaptureMouse) {
        io.MouseWheelH = static_cast<float>(dx);
        io.MouseWheel = static_cast<float>(dy);
    }
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
void UI::on_mouse_move(glm::vec2 pos) {
    auto& io = ImGui::GetIO();
    io.MousePos = ImVec2{pos.x, pos.y};
}

void UI::draw(const vk::UniqueCommandBuffer& cmd) {
    cmd->bindPipeline(vk::PipelineBindPoint::eGraphics, _gfx_pipeline.pipelines[0].get());
    cmd->bindDescriptorSets(
        vk::PipelineBindPoint::eGraphics,
        _gfx_pipeline.layout.get(),
        0,
        _descriptor_set,
        nullptr
    );
    cmd->bindVertexBuffers(0, _vertex.buffer(), {0});
    cmd->bindIndexBuffer(_index.buffer(), 0, vk::IndexType::eUint16);
    cmd->pushConstants(
        _gfx_pipeline.layout.get(),
        vk::ShaderStageFlagBits::eVertex,
        0,
        sizeof(PushConstantBlock),
        &_push_constants
    );

    auto* draw_data = ImGui::GetDrawData();
    i32 vertex_offset = 0;
    u32 index_offset = 0;

    for (i32 j = 0; j < draw_data->CmdListsCount; j++) {
        const auto* cmd_list = draw_data->CmdLists[j];
        for (int32_t k = 0; k < cmd_list->CmdBuffer.Size; k++) {
            const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[k];

            vk::Rect2D scissor_rect{};
            scissor_rect.offset.x = std::max(static_cast<int32_t>(pcmd->ClipRect.x), 0);
            scissor_rect.offset.y = std::max(static_cast<int32_t>(pcmd->ClipRect.y), 0);
            scissor_rect.extent.width = static_cast<uint32_t>(pcmd->ClipRect.z - pcmd->ClipRect.x);
            scissor_rect.extent.height = static_cast<uint32_t>(
                pcmd->ClipRect.w - pcmd->ClipRect.y
            );

            cmd->setScissor(0, scissor_rect);
            cmd->drawIndexed(pcmd->ElemCount, 1, index_offset, vertex_offset, 0);
            index_offset += pcmd->ElemCount;
        }
        vertex_offset += cmd_list->VtxBuffer.Size;
    }
}

void UI::build_pipeline() {
    // vertex input
    vk::VertexInputBindingDescription vertex_binding{0, 20, vk::VertexInputRate::eVertex};
    std::vector<vk::VertexInputAttributeDescription> vertex_attrs{
        {0, 0, vk::Format::eR32G32Sfloat, 0},
        {1, 0, vk::Format::eR32G32Sfloat, sizeof(f32) * 2},
        {2, 0, vk::Format::eR8G8B8A8Unorm, sizeof(f32) * 4},
    };

    vk::PushConstantRange pc_range{vk::ShaderStageFlagBits::eVertex, 0, sizeof(PushConstantBlock)};
    PipelineBuilder builder{};
    _gfx_pipeline =
        builder.add_push_constant(pc_range)
            .add_descriptor_set_layout(_descriptor_set_layout)
            .new_pipeline()
            .add_vertex_shader(ResourceManager::vertex_shader("ui"))
            .add_fragment_shader(ResourceManager::fragment_shader("ui"))
            .add_vertex_binding_description(vertex_binding)
            .add_vertex_attr_description(vertex_attrs)
            .with_polygon_mode(vk::PolygonMode::eFill)
            .with_cull_mode(vk::CullModeFlagBits::eNone)
            .with_front_face(vk::FrontFace::eCounterClockwise)
            .with_default_color_blend_transparency()
            .with_viewport(VulkanContext::swapchain().extent)
            .add_dynamic_state(vk::DynamicState::eScissor)
            .build(_render_pass);

    HVK_ASSERT(_gfx_pipeline.pipelines.size() == 1, "Should have created exactly one pipeline");
}

}  // namespace hvk
