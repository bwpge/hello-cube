// vulkan needs to be included before glfw
#include "hvk/engine.hpp"
#include <GLFW/glfw3.h>

#include "logger.hpp"

namespace hvk {

std::vector<const char*> get_extensions() {
    u32 count{};
    auto* glfw_ext = glfwGetRequiredInstanceExtensions(&count);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    auto result = std::vector(glfw_ext, glfw_ext + count);

    result.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

    return result;
}

void window_size_callback(GLFWwindow* window, i32 width, i32 height) {
    auto* engine = reinterpret_cast<Engine*>(glfwGetWindowUserPointer(window));
    engine->on_window_resize(width, height);
}

void window_pos_callback(GLFWwindow* window, i32 x, i32 y) {
    auto* engine = reinterpret_cast<Engine*>(glfwGetWindowUserPointer(window));
    engine->on_window_move(x, y);
}

void window_focus_callback(GLFWwindow* window, i32 focused) {
    if (focused) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    } else {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
    auto* engine = reinterpret_cast<Engine*>(glfwGetWindowUserPointer(window));
    engine->on_focus(static_cast<bool>(focused));
}

void key_callback(GLFWwindow* window, i32 key, i32, i32 action, i32 mods) {
    if (action == GLFW_PRESS) {
        auto* engine =
            reinterpret_cast<Engine*>(glfwGetWindowUserPointer(window));
        engine->on_key_press(key, mods);
    }
}

void scroll_callback(GLFWwindow* window, double dx, double dy) {
    auto* engine = reinterpret_cast<Engine*>(glfwGetWindowUserPointer(window));
    engine->on_scroll(dx, dy);
}

void framebufer_resize_callback(GLFWwindow* window, i32, i32) {
    auto* engine = reinterpret_cast<Engine*>(glfwGetWindowUserPointer(window));
    engine->on_resize();
}

void Engine::init() {
    if (_is_init) {
        spdlog::error("Attempted to initialize after already calling init()");
        return;
    }

    configure_logger();
    init_glfw();
    init_vulkan();

    // capture state for camera controls
    glfwGetCursorPos(_window.handle, &_cursor.x, &_cursor.y);
    _camera = Camera{45.f, VulkanContext::aspect(), 0.1f, 200.f};
    _focused = true;

    _is_init = true;
}

void Engine::run() {
    spdlog::info("Entering main application loop");

    _timer.reset();
    while (!glfwWindowShouldClose(_window.handle)) {
        glfwPollEvents();
        update(_timer.tick());
        render();
    }

    VulkanContext::device().waitIdle();
}

void Engine::update(double dt) {
    if (!_is_init || !_focused) {
        return;
    }

    // handle keyboard controls
    auto w = glfwGetKey(_window.handle, GLFW_KEY_W) == GLFW_PRESS;
    auto a = glfwGetKey(_window.handle, GLFW_KEY_A) == GLFW_PRESS;
    auto s = glfwGetKey(_window.handle, GLFW_KEY_S) == GLFW_PRESS;
    auto d = glfwGetKey(_window.handle, GLFW_KEY_D) == GLFW_PRESS;
    auto alt = glfwGetKey(_window.handle, GLFW_KEY_LEFT_ALT) == GLFW_PRESS;
    auto space = glfwGetKey(_window.handle, GLFW_KEY_SPACE) == GLFW_PRESS;
    auto shift = glfwGetKey(_window.handle, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS;

    if (shift) {
        _camera.set_sprint(true);
    } else {
        _camera.set_sprint(false);
    }
    if (w) {
        _camera.translate(CameraDirection::Forward, dt);
    }
    if (a) {
        _camera.translate(CameraDirection::Left, dt);
    }
    if (s) {
        _camera.translate(CameraDirection::Backward, dt);
    }
    if (d) {
        _camera.translate(CameraDirection::Right, dt);
    }
    if (alt) {
        _camera.translate(CameraDirection::Down, dt);
    }
    if (space) {
        _camera.translate(CameraDirection::Up, dt);
    }

    // handle mouse controls
    glm::dvec2 pos{};
    glfwGetCursorPos(_window.handle, &pos.x, &pos.y);
    if (pos != _cursor) {
        on_mouse_move(pos);
    }

    // DEBUG: rotate some meshes
    auto t = static_cast<float>(dt);
    auto& models = _scene.models();
    for (u32 i = 1; i < models.size(); i++) {
        models[i].rotate({-t, t, 0.0f});
    }
}

void Engine::render() {
    // aliases to make code below a bit more readable
    const auto& device = VulkanContext::device();
    const auto& swapchain = VulkanContext::swapchain();
    auto& frame = _frames[_frame_idx];
    auto& cmd = frame.cmd;
    const auto& graphics_queue = VulkanContext::graphics_queue();
    const auto& render_fence = frame.render_fence;
    const auto& render_semaphore = frame.render_semaphore;
    const auto& present_semaphore = frame.present_semaphore;
    const auto& pipeline = _pipelines.pipelines[_pipeline_idx];

    if (device.waitForFences(render_fence.get(), VK_TRUE, SYNC_TIMEOUT) !=
        vk::Result::eSuccess) {
        PANIC("Failed to wait for render fence");
    }

    auto next = device.acquireNextImageKHR(
        swapchain.handle.get(), SYNC_TIMEOUT, present_semaphore.get(), nullptr
    );
    if (_resized || next.result == vk::Result::eErrorOutOfDateKHR ||
        next.result == vk::Result::eSuboptimalKHR) {
        spdlog::debug("Window resized, re-creating swapchain");
        recreate_swapchain();
        return;
    }
    VKHPP_CHECK(next.result, "Failed to acquire next swapchain image");

    u32 idx = next.value;
    device.resetFences(render_fence.get());

    cmd->reset();
    cmd->begin(vk::CommandBufferBeginInfo{
        vk::CommandBufferUsageFlagBits::eOneTimeSubmit});

    vk::ClearValue color_clear{vk::ClearColorValue{0.1f, 0.1f, 0.1f, 1.0f}};
    vk::ClearValue depth_clear{vk::ClearDepthStencilValue{1.0f}};
    std::vector<vk::ClearValue> clear{color_clear, depth_clear};

    vk::RenderPassBeginInfo rpinfo{
        _render_pass.get(),
        _framebuffers[idx].get(),
        vk::Rect2D{{0, 0}, swapchain.extent},
        clear,
    };

    cmd->beginRenderPass(rpinfo, vk::SubpassContents::eInline);
    cmd->bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline.get());

    // bind descriptor sets
    cmd->bindDescriptorSets(
        vk::PipelineBindPoint::eGraphics,
        _pipelines.layout.get(),
        0,
        frame.descriptor,
        _scene_ubo.dyn_offset(_frame_idx)
    );
    cmd->bindDescriptorSets(
        vk::PipelineBindPoint::eGraphics,
        _pipelines.layout.get(),
        1,
        _texture_set,
        nullptr
    );

    auto camera = _camera.data();
    frame.camera_ubo.update(&camera);

    Material* current_material{};

    for (const auto& model : _scene.models()) {
        auto model_matrix = model.transform();
        auto constants = PushConstants{
            model_matrix,
            glm::transpose(glm::inverse(model_matrix)),
        };
        cmd->pushConstants(
            _pipelines.layout.get(),
            vk::ShaderStageFlagBits::eVertex,
            0,
            sizeof(PushConstants),
            &constants
        );
        for (const auto& node : model.nodes()) {
            if (current_material != node.material) {
                cmd->bindDescriptorSets(
                    vk::PipelineBindPoint::eGraphics,
                    _pipelines.layout.get(),
                    1,
                    node.material->descriptor_set,
                    nullptr
                );
            }
            model.draw_node(node, cmd);
        }
    }

    cmd->endRenderPass();
    cmd->end();

    vk::PipelineStageFlags mask =
        vk::PipelineStageFlagBits::eColorAttachmentOutput;
    vk::SubmitInfo submit{
        present_semaphore.get(),
        mask,
        cmd.get(),
        render_semaphore.get(),
    };
    graphics_queue.submit(submit, render_fence.get());

    vk::PresentInfoKHR present{
        render_semaphore.get(), swapchain.handle.get(), idx};
    VKHPP_CHECK(
        graphics_queue.presentKHR(present), "Failed to present swapchain frame"
    );

    _frame_count++;
    _frame_idx = _frame_count % _max_frames_in_flight;
}

void Engine::cleanup() {  // NOLINT(readability-make-member-function-const)
    spdlog::info("Shutdown requested, cleaning up");

    // vulkan resource cleanup is handled by vulkan-hpp,
    // destructors are called in reverse order of declaration

    spdlog::trace("Destroying window and terminating GLFW");
    if (_window.handle) {
        glfwDestroyWindow(_window.handle);
    }
    glfwTerminate();
    _is_init = false;
}

void Engine::cycle_pipeline() {
    _pipeline_idx = (_pipeline_idx + 1) % _pipelines.pipelines.size();
}

void Engine::toggle_fullscreen() {
    if (_window.is_fullscreen) {
        _window.is_fullscreen = false;
        glfwSetWindowMonitor(
            _window.handle,
            nullptr,
            _window.start_x,
            _window.start_y,
            _window.width,
            _window.height,
            _window.mode->refreshRate
        );
        return;
    }

    _window.is_fullscreen = true;
    glfwWindowHint(GLFW_RED_BITS, _window.mode->redBits);
    glfwWindowHint(GLFW_GREEN_BITS, _window.mode->greenBits);
    glfwWindowHint(GLFW_BLUE_BITS, _window.mode->blueBits);
    glfwWindowHint(GLFW_REFRESH_RATE, _window.mode->refreshRate);

    glfwSetWindowMonitor(
        _window.handle,
        _window.monitor,
        0,
        0,
        _window.mode->width,
        _window.mode->height,
        _window.mode->refreshRate
    );
}

void Engine::on_resize() {
    _resized = true;
}

void Engine::on_window_resize(i32 width, i32 height) {
    if (_window.is_fullscreen || width <= 0 || height <= 0) {
        return;
    }

    _window.width = width;
    _window.height = height;
}

void Engine::on_window_move(i32 x, i32 y) {
    if (_window.is_fullscreen) {
        return;
    }

    _window.start_x = x;
    _window.start_y = y;
}

void Engine::on_scroll(double, double dy) {
    if (!_is_init || !_focused) {
        return;
    }
    if (dy == 0.0) {
        return;
    }

    auto direction = dy > 0.0 ? ZoomDirection::In : ZoomDirection::Out;
    _camera.zoom(direction, _timer.elapsed_secs());
}

void Engine::on_key_press(i32 keycode, i32 mods) {
    if (!_is_init || !_focused) {
        return;
    }

    if ((mods & GLFW_MOD_ALT && keycode == GLFW_KEY_ENTER) ||
        keycode == GLFW_KEY_F11) {
        toggle_fullscreen();
    }

    switch (keycode) {
        case GLFW_KEY_ESCAPE:
            glfwSetWindowShouldClose(_window.handle, GLFW_TRUE);
            break;
        case GLFW_KEY_C:
            cycle_pipeline();
            break;
        case GLFW_KEY_R:
            _camera.reset();
            break;
        default:
            break;
    }
}

void Engine::on_focus(bool focused) {
    _focused = focused;
}

void Engine::on_mouse_move(glm::dvec2 pos) {
    if (!_is_init || !_focused) {
        return;
    }

    auto dx = pos.x - _cursor.x;
    auto dy = pos.y - _cursor.y;
    _cursor = pos;
    _camera.rotate(dx, dy);
}

void Engine::init_glfw() {
    spdlog::trace("Initializing GLFW");
    glfwInit();

    spdlog::trace(
        "Creating window: title='{}', width={}, height={}",
        _window.title,
        _window.width,
        _window.height
    );
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    _window.handle = glfwCreateWindow(
        _window.width, _window.height, _window.title.c_str(), nullptr, nullptr
    );
    if (!_window.handle) {
        PANIC("Failed to create window");
    }

    // capture initial state for fullscreen toggling
    _window.monitor = glfwGetPrimaryMonitor();
    _window.mode = glfwGetVideoMode(_window.monitor);
    glfwGetWindowPos(_window.handle, &_window.start_x, &_window.start_y);

    // center window
    auto x = (_window.mode->width / 2) - (_window.width / 2);
    auto y = (_window.mode->height / 2) - (_window.height / 2);
    glfwSetWindowPos(_window.handle, x, y);
    glfwGetWindowPos(_window.handle, &_window.start_x, &_window.start_y);

    // set window properties callbacks
    glfwSetInputMode(_window.handle, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    if (glfwRawMouseMotionSupported()) {
        glfwSetInputMode(_window.handle, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
    }
    glfwSetWindowSizeCallback(_window.handle, window_size_callback);
    glfwSetWindowPosCallback(_window.handle, window_pos_callback);
    glfwSetWindowFocusCallback(_window.handle, window_focus_callback);
    glfwSetKeyCallback(_window.handle, key_callback);
    glfwSetScrollCallback(_window.handle, scroll_callback);
    glfwSetFramebufferSizeCallback(_window.handle, framebufer_resize_callback);
    glfwSetWindowUserPointer(_window.handle, this);
}

void Engine::init_vulkan() {
    spdlog::trace("Initializing Vulkan");

    vk::ApplicationInfo info{
        "hello-vulkan",
        VK_MAKE_VERSION(0, 1, 0),
        "hvklib",
        VK_MAKE_VERSION(0, 1, 0),
        VK_API_VERSION_1_3,
    };

    // VulkanContext handles data structures that are accessed throughout the
    // application lifetime ("static"-ish) -- e.g., instance, device, swapchain,
    // etc. this is maybe not the best solution for thread safety, but makes a
    // lot of things much simpler, such as allocating buffers and images (which
    // require references to the device, queues, commands, and so on.)
    VulkanContext::init(_window.handle, info, get_extensions());
    spdlog::trace("Creating upload context");
    _upload_ctx = UploadContext{VulkanContext::queue_families().transfer};

    // load shaders
    std::vector<std::pair<std::string_view, ShaderType>> shaders{
        {"../shaders/mesh.vert.spv", ShaderType::Vertex},
        {"../shaders/mesh.vert.spv", ShaderType::Vertex},
        {"../shaders/mesh.frag.spv", ShaderType::Fragment},
        {"../shaders/wireframe.frag.spv", ShaderType::Fragment},
        {"../shaders/textured_lit.vert.spv", ShaderType::Vertex},
        {"../shaders/textured_lit.frag.spv", ShaderType::Fragment},
    };
    for (const auto& [path, type] : shaders) {
        ResourceManager::load_shader(path, type);
    }

    create_buffers();
    init_commands();
    init_renderpass();
    create_framebuffers();
    create_sync_obj();
    init_descriptors();
    create_pipelines();

    create_scene();
}

void Engine::create_buffers() {
    for (auto& frame : _frames) {
        frame.camera_ubo = Buffer{sizeof(CameraData)};
        frame.object_ssbo = Buffer{};
    }

    auto ubo_alignment = Buffer::pad_alignment(sizeof(SceneData));
    auto size = ubo_alignment * _max_frames_in_flight;
    _scene_ubo = Buffer{sizeof(SceneData), size};
    for (u32 i = 0; i < _max_frames_in_flight; i++) {
        auto data = _scene.data();
        _scene_ubo.update_indexed(&data, i);
    }
}

void Engine::create_scene() {
    {
        ResourceManager::load_image("../assets/uv-test.png", _upload_ctx);
        const auto* tex = ResourceManager::texture(
            {"uv-test", vk::Filter::eLinear, vk::SamplerAddressMode::eRepeat}
        );

        DescriptorSetWriter writer{};
        writer.write_images(
            _texture_set, _texture_bindings, {tex->create_image_info()}
        );
    }
    {
        auto model = Model::load_obj("../assets/sponza.obj", _upload_ctx);
        model.set_translation({0.0f, -2.0f, 0.0f});
        model.set_rotation({0.0f, glm::radians(90.0f), 0.0f});
        model.set_scale(0.02f);
        _scene.add_model(std::move(model));
    }

    auto* default_mat = ResourceManager::default_material();
    constexpr i32 count = 10;
    for (i32 i = -count; i <= count; i++) {
        for (i32 j = -count; j <= count; j++) {
            auto x = static_cast<float>(2 * i);
            auto y = std::abs(i + j) % 2 == 0 ? -0.25f : 0.25f;
            auto z = static_cast<float>(2 * j);

            Model model{};
            switch (std::abs(i + j) % 4) {
                case 0:
                    model = Model::cube(default_mat);
                    break;
                case 1:
                    model = Model::sphere(default_mat, 0.4f, 36, 20);
                    break;
                case 2:
                    model = Model::cylinder(default_mat, 0.35f, 0.85f, 30);
                    break;
                case 3:
                    model = Model::torus(default_mat, 0.5f, 0.2f, 20, 36);
            }

            model.set_translation({x, y, z});
            model.set_rotation({x, 0.0f, z});
            _scene.add_model(std::move(model));
        }
    }

    ResourceManager::prepare_materials(
        _desc_pool, _texture_set_layout, _texture_bindings
    );

    for (auto& model : _scene.models()) {
        model.upload(VulkanContext::graphics_queue(), _upload_ctx);
    }
}

void Engine::init_commands() {
    spdlog::trace("Initializing command buffers");
    const auto& device = VulkanContext::device();
    const auto& queue_family = VulkanContext::queue_families();

    for (auto& frame : _frames) {
        vk::CommandPoolCreateInfo cpci{
            vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
            queue_family.graphics,
        };
        frame.cmd_pool = device.createCommandPoolUnique(cpci);

        vk::CommandBufferAllocateInfo cbai{};
        cbai.setCommandPool(frame.cmd_pool.get())
            .setCommandBufferCount(1)
            .setLevel(vk::CommandBufferLevel::ePrimary);
        auto buffers = device.allocateCommandBuffersUnique(cbai);
        if (buffers.empty()) {
            PANIC("Failed to create command buffer");
        }
        frame.cmd = std::move(buffers[0]);
    }
}

void Engine::init_renderpass() {
    spdlog::trace("Initializing renderpass");

    vk::AttachmentDescription color_attach{
        {},
        VulkanContext::swapchain().format,
        vk::SampleCountFlagBits::e1,
        vk::AttachmentLoadOp::eClear,
        vk::AttachmentStoreOp::eStore,
        vk::AttachmentLoadOp::eDontCare,
        vk::AttachmentStoreOp::eDontCare,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::ePresentSrcKHR,
    };
    vk::AttachmentReference color_attach_ref{
        0, vk::ImageLayout::eAttachmentOptimal};

    vk::AttachmentDescription depth_attach{
        {},
        _depth_buffer.format(),
        vk::SampleCountFlagBits::e1,
        vk::AttachmentLoadOp::eClear,
        vk::AttachmentStoreOp::eStore,
        vk::AttachmentLoadOp::eLoad,
        vk::AttachmentStoreOp::eDontCare,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eDepthStencilAttachmentOptimal,
    };
    vk::AttachmentReference depth_attach_ref{
        1,
        vk::ImageLayout::eDepthAttachmentOptimal,
    };

    vk::SubpassDescription subpass{};
    subpass.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
        .setColorAttachments(color_attach_ref)
        .setPDepthStencilAttachment(&depth_attach_ref);

    vk::SubpassDependency color_dep{};
    color_dep.setSrcSubpass(VK_SUBPASS_EXTERNAL)
        .setDstSubpass(0)
        .setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
        .setSrcAccessMask(vk::AccessFlagBits::eNone)
        .setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
        .setDstAccessMask(vk::AccessFlagBits::eColorAttachmentWrite);

    vk::SubpassDependency depth_dep{};
    depth_dep.setSrcSubpass(VK_SUBPASS_EXTERNAL)
        .setDstSubpass(0)
        .setSrcStageMask(
            vk::PipelineStageFlagBits::eEarlyFragmentTests |
            vk::PipelineStageFlagBits::eLateFragmentTests
        )
        .setSrcAccessMask(vk::AccessFlagBits::eNone)
        .setDstStageMask(
            vk::PipelineStageFlagBits::eEarlyFragmentTests |
            vk::PipelineStageFlagBits::eLateFragmentTests
        )
        .setDstAccessMask(vk::AccessFlagBits::eDepthStencilAttachmentWrite);

    std::vector<vk::SubpassDependency> dependencies{color_dep, depth_dep};
    std::vector<vk::AttachmentDescription> attachments{
        color_attach, depth_attach};
    vk::RenderPassCreateInfo rpci{};
    rpci.setAttachments(attachments)
        .setSubpasses(subpass)
        .setDependencies(dependencies);

    _render_pass = VulkanContext::device().createRenderPassUnique(rpci);
}

void Engine::create_framebuffers() {
    const auto& device = VulkanContext::device();
    const auto& swapchain = VulkanContext::swapchain();

    spdlog::trace("Creating depth buffer");
    _depth_buffer = DepthBuffer{
        swapchain.extent,
    };

    spdlog::trace("Creating framebuffers");
    _framebuffers.clear();
    for (const auto& iv : swapchain.image_views) {
        auto attachments =
            std::vector<vk::ImageView>{iv.get(), _depth_buffer.image_view()};

        vk::FramebufferCreateInfo info{};
        info.setRenderPass(_render_pass.get())
            .setAttachments(attachments)
            .setWidth(swapchain.extent.width)
            .setHeight(swapchain.extent.height)
            .setLayers(1);

        _framebuffers.push_back(device.createFramebufferUnique(info));
    }
}

void Engine::init_descriptors() {
    const auto& device = VulkanContext::device();

    std::vector<vk::DescriptorPoolSize> pool_sizes{
        {vk::DescriptorType::eUniformBuffer, 10},
        {vk::DescriptorType::eUniformBufferDynamic, 10},
        {vk::DescriptorType::eCombinedImageSampler, 10},
    };
    vk::DescriptorPoolCreateInfo pool_info{};
    // TODO(bwpge): properly calculate max sets
    pool_info.setMaxSets(1000).setPoolSizes(pool_sizes);
    _desc_pool = device.createDescriptorPoolUnique(pool_info);

    _frame_bindings = DescriptorSetBindingMap{
        {vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex},
        {vk::DescriptorType::eUniformBufferDynamic,
         vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment},
    };
    _texture_bindings = DescriptorSetBindingMap{
        {vk::DescriptorType::eCombinedImageSampler,
         vk::ShaderStageFlagBits::eFragment},
    };

    _global_desc_set_layout = _frame_bindings.build_layout();
    _texture_set_layout = _texture_bindings.build_layout();
    _texture_set =
        VulkanContext::allocate_descriptor_set(_desc_pool, _texture_set_layout);

    for (auto& frame : _frames) {
        // allocate the descriptor sets
        frame.descriptor = VulkanContext::allocate_descriptor_set(
            _desc_pool, _global_desc_set_layout
        );

        // write the appropriate descriptors
        DescriptorSetWriter writer{};
        writer.write_buffers(
            frame.descriptor,
            _frame_bindings,
            {
                frame.camera_ubo.descriptor_buffer_info(),
                _scene_ubo.descriptor_buffer_info(),
            }
        );
    }
}

void Engine::create_sync_obj() {
    spdlog::trace("Creating synchronization structures");
    const auto& device = VulkanContext::device();

    for (auto& frame : _frames) {
        frame.render_fence = device.createFenceUnique(vk::FenceCreateInfo{
            vk::FenceCreateFlagBits::eSignaled});
        frame.present_semaphore = device.createSemaphoreUnique({});
        frame.render_semaphore = device.createSemaphoreUnique({});
    }
}

void Engine::create_pipelines() {
    spdlog::trace("Creating graphics pipelines");
    const auto& swapchain = VulkanContext::swapchain();

    // hardcoded push constants for matrices
    vk::PushConstantRange push_constant{};
    push_constant.setOffset(0)
        .setSize(sizeof(PushConstants))
        .setStageFlags(vk::ShaderStageFlagBits::eVertex);

    PipelineBuilder builder{};
    _pipelines =
        builder.add_push_constant(push_constant)
            .add_descriptor_set_layout(_global_desc_set_layout)
            .add_descriptor_set_layout(_texture_set_layout)
            // textured pipeline
            .new_pipeline()
            .add_vertex_shader(ResourceManager::vertex_shader("textured_lit"))
            .add_fragment_shader(ResourceManager::fragment_shader("textured_lit"
            ))
            .add_vertex_binding_description(Vertex::binding_desc())
            .add_vertex_attr_description(Vertex::attr_desc())
            .with_default_color_blend_transparency()
            .with_default_viewport(swapchain.extent)
            .with_depth_stencil(true, true, vk::CompareOp::eLessOrEqual)
            // debug pipeline
            .new_pipeline()
            .add_vertex_shader(ResourceManager::vertex_shader("mesh"))
            .add_fragment_shader(ResourceManager::fragment_shader("mesh"))
            .add_vertex_binding_description(Vertex::binding_desc())
            .add_vertex_attr_description(Vertex::attr_desc())
            .with_default_viewport(swapchain.extent)
            .with_depth_stencil(true, true, vk::CompareOp::eLessOrEqual)
            // wireframe pipeline
            .new_pipeline()
            .add_vertex_shader(ResourceManager::vertex_shader("mesh"))
            .add_fragment_shader(ResourceManager::fragment_shader("wireframe"))
            .add_vertex_binding_description(Vertex::binding_desc())
            .add_vertex_attr_description(Vertex::attr_desc())
            .with_default_viewport(swapchain.extent)
            .with_polygon_mode(vk::PolygonMode::eLine)
            .with_cull_mode(vk::CullModeFlagBits::eNone)
            // build all pipelines with this layout
            .build(_render_pass.get());
}

void Engine::recreate_swapchain() {
    _resized = false;

    // HACK: spin lock minimized state
    // this doesn't allow anything else in the application to execute
    i32 width{};
    i32 height{};
    glfwGetFramebufferSize(_window.handle, &width, &height);
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(_window.handle, &width, &height);
        glfwPollEvents();
    }

    // when re-creating the swapchain, we might have frames being presented
    // or commands still being executed. device->waitIdle is a bit of a brute
    // force solution (it will wait on every queue owned by the device to be
    // idle), but this guarantees we can re-create sync objects without having
    // to worry about what is still in use. re-creating them is much easier
    // than trying to reuse them, and this is not critical path code.
    //
    // for background info, see:
    //   - https://stackoverflow.com/questions/59825832
    //   - https://stackoverflow.com/questions/70762372
    //   - https://github.com/KhronosGroup/Vulkan-Docs/issues/1059
    VulkanContext::device().waitIdle();

    _framebuffers.clear();
    VulkanContext::instance().build_swapchain(_window.handle);
    create_framebuffers();
    create_sync_obj();
    create_pipelines();

    _camera.set_aspect(VulkanContext::aspect());
}

}  // namespace hvk
