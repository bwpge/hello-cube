// vulkan needs to be included before glfw
#include "engine.hpp"
#include <GLFW/glfw3.h>

#include "logger.hpp"

#define SYNC_TIMEOUT 1000000000

namespace hc {

std::vector<const char*> get_extensions() {
    u32 count{};
    auto* glfw_ext = glfwGetRequiredInstanceExtensions(&count);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    auto result = std::vector(glfw_ext, glfw_ext + count);

    result.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

    return result;
}

vk::Extent2D get_surface_extent(
    vk::PhysicalDevice& device,
    vk::SurfaceKHR& surface,
    GLFWwindow* window
) {
    const auto capabilities = device.getSurfaceCapabilitiesKHR(surface);

    vk::Extent2D extent{};
    if (capabilities.currentExtent.width != std::numeric_limits<u32>::max()) {
        extent = capabilities.currentExtent;
    } else {
        i32 width{};
        i32 height{};
        glfwGetFramebufferSize(window, &width, &height);
        extent =
            vk::Extent2D{static_cast<u32>(width), static_cast<u32>(height)};
    }
    extent.width = std::clamp(
        extent.width,
        capabilities.minImageExtent.width,
        capabilities.maxImageExtent.width
    );
    extent.height = std::clamp(
        extent.height,
        capabilities.minImageExtent.height,
        capabilities.maxImageExtent.height
    );

    return extent;
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
    _camera = Camera{45.f, aspect_ratio(), 0.1f, 200.f};
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

    _device->waitIdle();
}

void Engine::update(double dt) {
    if (!_is_init || !_focused) {
        return;
    }

    // orbit light position around origin
    auto l_theta = static_cast<float>(_timer.total_secs()) * glm::pi<float>();
    auto l_x = 30.0f * glm::sin(l_theta);
    auto l_z = 30.0f * glm::cos(l_theta);
    _scene.set_light_pos({l_x, 3.0f, l_z});

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
        _camera.move(CameraDirection::Forward, dt);
    }
    if (a) {
        _camera.move(CameraDirection::Left, dt);
    }
    if (s) {
        _camera.move(CameraDirection::Backward, dt);
    }
    if (d) {
        _camera.move(CameraDirection::Right, dt);
    }
    if (alt) {
        _camera.move(CameraDirection::Down, dt);
    }
    if (space) {
        _camera.move(CameraDirection::Up, dt);
    }

    glm::dvec2 pos{};
    glfwGetCursorPos(_window.handle, &pos.x, &pos.y);
    if (pos != _cursor) {
        on_mouse_move(pos);
    }
}

void Engine::render() {
    // aliases to make code below a bit more readable
    auto& frame = _frames[_frame_idx];
    auto& cmd = frame.cmd;
    const auto& render_fence = frame.render_fence;
    const auto& render_semaphore = frame.render_semaphore;
    const auto& present_semaphore = frame.present_semaphore;

    if (_device->waitForFences(render_fence.get(), VK_TRUE, SYNC_TIMEOUT) !=
        vk::Result::eSuccess) {
        PANIC("Failed to wait for render fence");
    }

    auto res = _device->acquireNextImageKHR(
        _swapchain.handle.get(), SYNC_TIMEOUT, present_semaphore.get(), nullptr
    );
    if (_resized || res.result == vk::Result::eErrorOutOfDateKHR ||
        res.result == vk::Result::eSuboptimalKHR) {
        spdlog::debug("Window resized, re-creating swapchain");
        recreate_swapchain();
        return;
    }
    if (res.result != vk::Result::eSuccess) {
        PANIC("Failed to acquire next swapchain image");
    }
    u32 idx = res.value;
    _device->resetFences(render_fence.get());

    cmd->reset();
    cmd->begin(vk::CommandBufferBeginInfo{
        vk::CommandBufferUsageFlagBits::eOneTimeSubmit});

    vk::ClearValue color_clear{vk::ClearColorValue{0.0f, 0.0f, 0.2f, 1.0f}};
    vk::ClearValue depth_clear{vk::ClearDepthStencilValue{1.0f}};
    std::vector<vk::ClearValue> clear{color_clear, depth_clear};

    vk::RenderPassBeginInfo rpinfo{
        _render_pass.get(),
        _framebuffers[idx].get(),
        vk::Rect2D{{0, 0}, _swapchain.extent},
        clear,
    };

    cmd->beginRenderPass(rpinfo, vk::SubpassContents::eInline);
    cmd->bindPipeline(
        vk::PipelineBindPoint::eGraphics,
        _gfx_pipelines.pipelines[_pipeline_idx].get()
    );

    auto proj = _camera.projection();
    auto view = _camera.view();

    auto constants = PushConstants{proj, view, {}, _scene.light_pos()};
    for (const auto& mesh : _scene.meshes()) {
        constants.model = mesh.transform();

        cmd->pushConstants(
            _gfx_pipelines.layout.get(),
            vk::ShaderStageFlagBits::eVertex,
            0,
            sizeof(PushConstants),
            &constants
        );
        mesh.bind(cmd.get());
        mesh.draw(cmd);
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
    _graphics_queue.submit(submit, render_fence.get());

    vk::PresentInfoKHR present{
        render_semaphore.get(), _swapchain.handle.get(), idx};
    if (_graphics_queue.presentKHR(present) != vk::Result::eSuccess) {
        PANIC("Failed to present swapchain frame");
    }

    _frame_count++;
    _frame_idx = _frame_count % _max_frames_in_flight;
}

void Engine::cleanup() {
    spdlog::info("Shutdown requested, cleaning up");

    // vulkan resource cleanup is handled by vulkan-hpp,
    // destructors are called in reverse order of declaration

    _depth_buffer.destroy();

    spdlog::trace("Destroying scene");
    _scene.destroy();

    spdlog::trace("Destroying allocator");
    vmaDestroyAllocator(_allocator);

    spdlog::trace("Destroying window and terminating GLFW");
    if (_window.handle) {
        glfwDestroyWindow(_window.handle);
    }
    glfwTerminate();
}

void Engine::cycle_pipeline() {
    _pipeline_idx = (_pipeline_idx + 1) % _gfx_pipelines.pipelines.size();
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
    _window.is_fullscreen = true;
}

void Engine::on_resize() {
    _resized = true;
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

float Engine::aspect_ratio() const noexcept {
    if (_window.height == 0) {
        return 0.0f;
    }

    return static_cast<float>(_swapchain.extent.width) /
           static_cast<float>(_swapchain.extent.height);
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
    glfwSetWindowFocusCallback(_window.handle, window_focus_callback);
    glfwSetKeyCallback(_window.handle, key_callback);
    glfwSetScrollCallback(_window.handle, scroll_callback);
    glfwSetFramebufferSizeCallback(_window.handle, framebufer_resize_callback);
    glfwSetWindowUserPointer(_window.handle, this);
}

void Engine::init_vulkan() {
    spdlog::trace("Initializing Vulkan");

    create_instance();
    create_surface();
    create_device();
    init_allocator();
    init_commands();
    load_shaders();
    create_swapchain();
    create_scene();
    init_renderpass();
    create_framebuffers();
    create_sync_obj();
    create_pipelines();
}

void Engine::init_allocator() {
    VmaAllocatorCreateInfo info{};
    info.physicalDevice = _gpu;
    info.device = _device.get();
    info.instance = _instance.get();
    info.vulkanApiVersion = VK_API_VERSION_1_3;
    vmaCreateAllocator(&info, &_allocator);
}

void Engine::create_instance() {
    spdlog::trace("Creating Vulkan instance");

    vk::ApplicationInfo ai{
        "hello-cube",
        VK_MAKE_VERSION(0, 1, 0),
        "hc",
        VK_MAKE_VERSION(0, 1, 0),
        VK_API_VERSION_1_3,
    };
    vk::InstanceCreateInfo ici{{}, &ai};

    auto dmci = vk::DebugUtilsMessengerCreateInfoEXT{
        {},
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo |
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
        vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
            vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
            vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
            vk::DebugUtilsMessageTypeFlagBitsEXT::eDeviceAddressBinding,
        hc::debug_callback,
    };

    auto extensions = get_extensions();
    auto layers = std::vector<const char*>{
        "VK_LAYER_KHRONOS_validation", "VK_LAYER_KHRONOS_synchronization2"};

    spdlog::debug(
        "Requested instance extensions: [{}]",
        spdlog::fmt_lib::join(extensions, ", ")
    );
    spdlog::debug(
        "Requested instance extensions: [{}]",
        spdlog::fmt_lib::join(layers, ", ")
    );

    ici.setPEnabledExtensionNames(extensions)
        .setPEnabledLayerNames(layers)
        .setPNext(&dmci);

    _instance = vk::createInstanceUnique(ici);
    _messenger = _instance->createDebugUtilsMessengerEXTUnique(dmci);
}

void Engine::create_surface() {
    spdlog::trace("Initializing window surface");

    VkSurfaceKHR surface{};
    if (glfwCreateWindowSurface(
            _instance.get(), _window.handle, nullptr, &surface
        ) != VK_SUCCESS) {
        PANIC("Failed to create window surface");
    }
    if (surface == VK_NULL_HANDLE) {
        PANIC("Failed to create window surface");
    } else {
        // https://github.com/KhronosGroup/Vulkan-Hpp/blob/e2f5348e28ba22dd9b87028f2d9bb7d556aa7b6e/samples/utils/utils.cpp#L790-L798
        auto deleter =
            vk::ObjectDestroy<vk::Instance, VULKAN_HPP_DEFAULT_DISPATCHER_TYPE>(
                _instance.get()
            );
        _surface = vk::UniqueSurfaceKHR{surface, deleter};
    }
}

void Engine::create_device() {
    spdlog::trace("Selecting physical device");

    const auto devices = _instance->enumeratePhysicalDevices();
    spdlog::debug(
        "Found {} {}",
        devices.size(),
        devices.size() == 1 ? "device" : "devices"
    );

    i32 selected = -1;
    for (i32 i = 0; i < static_cast<i32>(devices.size()); i++) {
        auto p = devices[i].getProperties();
        if (p.deviceType == vk::PhysicalDeviceType::eDiscreteGpu) {
            selected = i;
            spdlog::debug(
                "Selected device '{}'", static_cast<const char*>(p.deviceName)
            );
            _gpu = devices[selected];
            break;
        }
    }
    if (selected < 0) {
        PANIC("Failed to locate a suitable device");
    }

    _gpu = devices[selected];

    auto queues = _gpu.getQueueFamilyProperties();
    auto idx_graphics = std::optional<u32>{};
    auto idx_present = std::optional<u32>();
    for (u32 i = 0; i < static_cast<u32>(queues.size()); i++) {
        const auto props = queues[i];
        const auto has_present = _gpu.getSurfaceSupportKHR(i, _surface.get());
        if (props.queueFlags & vk::QueueFlagBits::eGraphics) {
            idx_graphics = i;
        }
        if (has_present) {
            idx_present = i;
        }
        if (idx_graphics.has_value() && idx_present.has_value()) {
            break;
        }
    }
    if (!idx_graphics.has_value() || !idx_present.has_value()) {
        PANIC("Failed to locate required queue indices");
    }
    _queue_family.graphics = idx_graphics.value();
    _queue_family.present = idx_present.value();
    spdlog::debug(
        "Located queue indices:\n    - Graphics: {}\n    - Present: {}",
        _queue_family.graphics,
        _queue_family.present
    );

    f32 priority = 1.0f;
    auto unique_idx =
        std::set<u32>{_queue_family.graphics, _queue_family.present};
    auto infos = std::vector<vk::DeviceQueueCreateInfo>{};

    for (u32 idx : unique_idx) {
        vk::DeviceQueueCreateInfo dqci{{}, idx, 1, &priority, nullptr};
        infos.push_back(dqci);
    }

    vk::PhysicalDeviceFeatures features{};
    features.setFillModeNonSolid(VK_TRUE);

    // https://stackoverflow.com/questions/73746051/vulkan-how-to-enable-synchronization-2-feature
    // need to enable synchronization2 feature to use
    // VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL
    vk::PhysicalDeviceSynchronization2Features pdsf{};
    pdsf.setSynchronization2(VK_TRUE);

    // https://vulkan.lunarg.com/doc/view/1.3.250.1/windows/1.3-extensions/vkspec.html#VUID-VkAttachmentReference2-separateDepthStencilLayouts-03313
    // need to enable to use VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL in depth
    // attachment for render subpass
    vk::PhysicalDeviceVulkan12Features pdv12f{};
    pdv12f.setSeparateDepthStencilLayouts(VK_TRUE).setPNext(&pdsf);

    vk::DeviceCreateInfo dci{};
    auto extensions = std::vector{VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    dci.setQueueCreateInfos(infos)
        .setPEnabledFeatures(&features)
        .setPEnabledExtensionNames(extensions)
        .setPNext(&pdv12f);

    _device = _gpu.createDeviceUnique(dci);
    _graphics_queue = _device->getQueue(_queue_family.graphics, 0);
}

void Engine::load_shaders() {
    spdlog::debug("Loading shaders");

    _shaders.vertex["mesh"] = Shader::load_spv("../shaders/mesh.vert.spv");
    _shaders.fragment["mesh"] = Shader::load_spv("../shaders/mesh.frag.spv");
    _shaders.fragment["hello_triangle"] =
        Shader::load_spv("../shaders/hello_triangle.frag.spv");
    _shaders.vertex["hello_triangle"] =
        Shader::load_spv("../shaders/hello_triangle.vert.spv");
    _shaders.fragment["wireframe"] =
        Shader::load_spv("../shaders/wireframe.frag.spv");
}

void Engine::create_swapchain() {
    spdlog::trace("Creating swapchain");

    const auto capabilities = _gpu.getSurfaceCapabilitiesKHR(_surface.get());
    _swapchain.extent =
        get_surface_extent(_gpu, _surface.get(), _window.handle);
    _swapchain.format = vk::Format::eB8G8R8A8Unorm;
    auto color = vk::ColorSpaceKHR::eSrgbNonlinear;
    auto mode = vk::PresentModeKHR::eMailbox;

    u32 img_count = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount > 0 &&
        img_count > capabilities.maxImageCount) {
        img_count = capabilities.maxImageCount;
    }

    vk::SwapchainCreateInfoKHR scci{
        {},
        _surface.get(),
        img_count,
        _swapchain.format,
        color,
        _swapchain.extent,
        1,
        vk::ImageUsageFlagBits::eColorAttachment,
    };
    if (_queue_family.graphics != _queue_family.present) {
        auto indices =
            std::vector{_queue_family.graphics, _queue_family.present};
        scci.setImageSharingMode(vk::SharingMode::eConcurrent)
            .setQueueFamilyIndices(indices);
    } else {
        scci.setImageSharingMode(vk::SharingMode::eExclusive);
    }
    scci.setPreTransform(capabilities.currentTransform)
        .setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque)
        .setPresentMode(mode)
        .setClipped(VK_TRUE);

    _swapchain.handle = _device->createSwapchainKHRUnique(scci);
    _swapchain.images = _device->getSwapchainImagesKHR(_swapchain.handle.get());

    _swapchain.image_views.clear();
    for (auto img : _swapchain.images) {
        vk::ImageViewCreateInfo ivci{};
        ivci.setImage(img)
            .setViewType(vk::ImageViewType::e2D)
            .setFormat(_swapchain.format)
            .setSubresourceRange({vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1});
        _swapchain.image_views.push_back(_device->createImageViewUnique(ivci));
    }

    _depth_buffer.destroy();
    _depth_buffer = DepthBuffer{_allocator, _device.get(), _swapchain.extent};
}

void Engine::create_scene() {
    {
        auto& mesh = _scene.add_mesh(
            Mesh::load_obj(_allocator, "../assets/monkey_smooth.obj")
        );
        mesh.set_translation({0.0f, 3.0f, -3.0f});
        mesh.set_scale(1.25f);
    }

    const i32 count = 20;
    for (i32 i = -count; i <= count; i++) {
        for (i32 j = -count; j <= count; j++) {
            auto x = static_cast<float>(2 * i);
            auto y = std::abs(i + j) % 2 == 0 ? -0.25f : 0.25f;
            auto z = static_cast<float>(2 * j);
            auto r = static_cast<float>(std::abs(count + i)) / (2 * count);
            auto g = static_cast<float>(std::abs(count + j)) / (2 * count);
            auto color = glm::vec3{r, g, 1.0f};

            auto& mesh =
                std::abs(j) % 2 == 1
                    ? _scene.add_mesh(
                          Mesh::sphere(_allocator, 0.4f, color, 36, 20)
                      )
                    : _scene.add_mesh(Mesh::cube(_allocator, 0.5f, color));
            mesh.set_translation({x, y, z});
            mesh.set_rotation({x, 0.0f, z});
        }
    }

    for (auto& mesh : _scene.meshes()) {
        mesh.upload();
    }
}

void Engine::init_commands() {
    spdlog::trace("Initializing command buffers");

    for (auto& frame : _frames) {
        vk::CommandPoolCreateInfo cpci{
            vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
            _queue_family.graphics,
        };
        frame.cmd_pool = _device->createCommandPoolUnique(cpci);

        vk::CommandBufferAllocateInfo cbai{};
        cbai.setCommandPool(frame.cmd_pool.get())
            .setCommandBufferCount(1)
            .setLevel(vk::CommandBufferLevel::ePrimary);
        auto buffers = _device->allocateCommandBuffersUnique(cbai);
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
        _swapchain.format,
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

    _render_pass = _device->createRenderPassUnique(rpci);
}

void Engine::create_framebuffers() {
    spdlog::trace("Creating framebuffers");

    _framebuffers.clear();
    for (const auto& iv : _swapchain.image_views) {
        auto attachments =
            std::vector<vk::ImageView>{iv.get(), _depth_buffer.image_view()};

        vk::FramebufferCreateInfo info{};
        info.setRenderPass(_render_pass.get())
            .setAttachments(attachments)
            .setWidth(_swapchain.extent.width)
            .setHeight(_swapchain.extent.height)
            .setLayers(1);

        _framebuffers.push_back(_device->createFramebufferUnique(info));
    }
}

void Engine::create_sync_obj() {
    spdlog::trace("Creating synchronization structures");

    for (auto& frame : _frames) {
        frame.render_fence = _device->createFenceUnique(vk::FenceCreateInfo{
            vk::FenceCreateFlagBits::eSignaled});
        frame.present_semaphore = _device->createSemaphoreUnique({});
        frame.render_semaphore = _device->createSemaphoreUnique({});
    }
}

void Engine::create_pipelines() {
    spdlog::trace("Creating graphics pipelines");

    // hardcoded push constants for matrices
    vk::PushConstantRange push_constant{};
    push_constant.setOffset(0)
        .setSize(sizeof(PushConstants))
        .setStageFlags(vk::ShaderStageFlagBits::eVertex);

    PipelineBuilder builder{};
    _gfx_pipelines =
        builder.set_push_constant(push_constant)
            .new_pipeline()
            .add_vertex_shader(
                _shaders.vertex["mesh"].shader_module(_device.get())
            )
            .add_fragment_shader(
                _shaders.fragment["mesh"].shader_module(_device.get())
            )
            .set_extent(_swapchain.extent)
            .set_cull_mode(vk::CullModeFlagBits::eBack)
            .set_depth_stencil(true, true, vk::CompareOp::eLessOrEqual)
            .new_pipeline()
            .add_vertex_shader(
                _shaders.vertex["mesh"].shader_module(_device.get())
            )
            .add_fragment_shader(
                _shaders.fragment["wireframe"].shader_module(_device.get())
            )
            .set_extent(_swapchain.extent)
            .set_polygon_mode(vk::PolygonMode::eLine)
            .set_cull_mode(vk::CullModeFlagBits::eNone)
            .build(_device.get(), _render_pass.get());
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
    _device->waitIdle();

    destroy_swapchain();
    create_swapchain();
    create_framebuffers();
    create_sync_obj();
    create_pipelines();

    _camera.set_aspect(aspect_ratio());
}

void Engine::destroy_swapchain() {
    _framebuffers.clear();
    _swapchain.image_views.clear();
    _swapchain.images.clear();
    _swapchain.handle = {};
}

}  // namespace hc
