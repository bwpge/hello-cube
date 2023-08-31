// vulkan needs to be included before glfw
#include "engine.hpp"
#include <GLFW/glfw3.h>

#include "logger.hpp"

#define SYNC_TIMEOUT 1000000000

namespace hc {

auto get_extensions() -> std::vector<const char*> {
    u32 count{};
    auto* glfw_ext = glfwGetRequiredInstanceExtensions(&count);
    auto result = std::vector(glfw_ext, glfw_ext + count);

    result.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

    return result;
}

auto get_surface_extent(
    vk::PhysicalDevice& device,
    vk::SurfaceKHR& surface,
    GLFWwindow* window
) -> vk::Extent2D {
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

void key_callback(GLFWwindow* window, i32 key, i32, i32 action, i32) {
    if (action == GLFW_PRESS) {
        auto* engine =
            reinterpret_cast<Engine*>(glfwGetWindowUserPointer(window));
        engine->on_key_press(key);
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

    hc::configure_logger();

    spdlog::trace("Initializing GLFW");
    glfwInit();

    spdlog::trace(
        "Creating window: title='{}', width={}, height={}",
        _title,
        _width,
        _height
    );
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    _window =
        glfwCreateWindow(_width, _height, _title.c_str(), nullptr, nullptr);
    glfwSetWindowUserPointer(_window, this);

    if (!_window) {
        PANIC("Failed to create window");
    }

    _camera = Camera{45.f, aspect_ratio(), 0.1f, 200.f};
    _focused = true;

    glfwSetInputMode(_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    if (glfwRawMouseMotionSupported()) {
        glfwSetInputMode(_window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
    }
    glfwSetWindowFocusCallback(_window, window_focus_callback);
    glfwSetKeyCallback(_window, key_callback);
    glfwSetScrollCallback(_window, scroll_callback);
    glfwSetFramebufferSizeCallback(_window, framebufer_resize_callback);

    init_vulkan();
    glfwGetCursorPos(_window, &_cursor.x, &_cursor.y);

    _is_init = true;
}

void Engine::run() {
    spdlog::info("Entering main application loop");

    _timer.reset();
    while (!glfwWindowShouldClose(_window)) {
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

    auto w = glfwGetKey(_window, GLFW_KEY_W) == GLFW_PRESS;
    auto a = glfwGetKey(_window, GLFW_KEY_A) == GLFW_PRESS;
    auto s = glfwGetKey(_window, GLFW_KEY_S) == GLFW_PRESS;
    auto d = glfwGetKey(_window, GLFW_KEY_D) == GLFW_PRESS;
    auto alt = glfwGetKey(_window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS;
    auto space = glfwGetKey(_window, GLFW_KEY_SPACE) == GLFW_PRESS;

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
    glfwGetCursorPos(_window, &pos.x, &pos.y);
    if (pos != _cursor) {
        on_mouse_move(pos, dt);
    }
}

void Engine::render() {
    if (_device->waitForFences(_render_fence.get(), VK_TRUE, SYNC_TIMEOUT) !=
        vk::Result::eSuccess) {
        PANIC("Failed to wait for render fence");
    }

    auto res = _device->acquireNextImageKHR(
        _swapchain.get(), SYNC_TIMEOUT, _present_semaphore.get(), nullptr
    );
    if (_resized || res.result == vk::Result::eErrorOutOfDateKHR ||
        res.result == vk::Result::eSuboptimalKHR) {
        spdlog::debug("Window resized, re-creating swapchain");
        _resized = false;
        recreate_swapchain();
        return;
    }
    if (res.result != vk::Result::eSuccess) {
        PANIC("Failed to acquire next swapchain image");
    }
    u32 idx = res.value;
    _device->resetFences(_render_fence.get());

    _cmd_buffer->reset();
    _cmd_buffer->begin(vk::CommandBufferBeginInfo{
        vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
    vk::ClearValue clear{vk::ClearColorValue{0.0f, 0.0f, 0.2f, 1.0f}};
    vk::RenderPassBeginInfo rpinfo{
        _render_pass.get(),
        _framebuffers[idx].get(),
        vk::Rect2D{{0, 0}, _swapchain_extent},
        clear,
    };

    _cmd_buffer->beginRenderPass(rpinfo, vk::SubpassContents::eInline);
    _cmd_buffer->bindPipeline(
        vk::PipelineBindPoint::eGraphics,
        _gfx_pipelines.pipelines[_pipeline_idx].get()
    );

    auto proj = _camera.get_projection();
    auto view = _camera.get_view();
    // TODO(bwpge): fix hardcoded transform
    auto model = _meshes[0].get_transform();
    auto constants = PushConstants{proj, view, model};

    _cmd_buffer->pushConstants(
        _gfx_pipelines.layout.get(),
        vk::ShaderStageFlagBits::eVertex,
        0,
        sizeof(PushConstants),
        &constants
    );
    for (const auto& mesh : _meshes) {
        mesh.bind(_cmd_buffer.get());
        mesh.draw(_cmd_buffer);
    }

    _cmd_buffer->endRenderPass();
    _cmd_buffer->end();

    vk::PipelineStageFlags mask =
        vk::PipelineStageFlagBits::eColorAttachmentOutput;
    vk::SubmitInfo submit{
        _present_semaphore.get(),
        mask,
        _cmd_buffer.get(),
        _render_semaphore.get(),
    };
    _graphics_queue.submit(submit, _render_fence.get());

    vk::PresentInfoKHR present{_render_semaphore.get(), _swapchain.get(), idx};
    if (_graphics_queue.presentKHR(present) != vk::Result::eSuccess) {
        PANIC("Failed to present swapchain frame");
    }

    _frame_number++;
}

void Engine::cleanup() {
    spdlog::info("Shutdown requested, cleaning up");

    // vulkan resource cleanup is handled by vulkan-hpp,
    // destructors are called in reverse order of declaration

    spdlog::trace("Destroying meshes");
    for (auto& mesh : _meshes) {
        mesh.destroy();
    }
    spdlog::trace("Destroying allocator");
    vmaDestroyAllocator(_allocator);

    spdlog::trace("Destroying window and terminating GLFW");
    if (_window) {
        glfwDestroyWindow(_window);
    }
    glfwTerminate();
}

void Engine::cycle_pipeline() {
    _pipeline_idx = (_pipeline_idx + 1) % _gfx_pipelines.pipelines.size();
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

void Engine::on_key_press(i32 keycode) {
    if (!_is_init || !_focused) {
        return;
    }

    switch (keycode) {
        case GLFW_KEY_ESCAPE:
            glfwSetWindowShouldClose(_window, GLFW_TRUE);
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

void Engine::on_mouse_move(glm::dvec2 pos, double dt) {
    if (!_is_init || !_focused) {
        return;
    }

    auto dx = pos.x - _cursor.x;
    auto dy = pos.y - _cursor.y;
    _cursor = pos;
    _camera.rotate(dx, dy, dt);
}

float Engine::aspect_ratio() const noexcept {
    return _height == 0
               ? 0.f
               : static_cast<float>(_width) / static_cast<float>(_height);
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
    load_meshes();
    init_renderpass();
    create_framebuffers();
    init_sync_obj();
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

    VkSurfaceKHR surface;
    if (glfwCreateWindowSurface(_instance.get(), _window, nullptr, &surface) !=
        VK_SUCCESS) {
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

    vk::DeviceCreateInfo dci{};
    auto extensions = std::vector{VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    dci.setQueueCreateInfos(infos)
        .setPEnabledFeatures(&features)
        .setPEnabledExtensionNames(extensions)
        .setPNext(&pdsf);

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
    _swapchain_extent = get_surface_extent(_gpu, _surface.get(), _window);
    _swapchain_format = vk::Format::eB8G8R8A8Unorm;
    auto color = vk::ColorSpaceKHR::eSrgbNonlinear;
    auto mode = vk::PresentModeKHR::eFifo;

    u32 img_count = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount > 0 &&
        img_count > capabilities.maxImageCount) {
        img_count = capabilities.maxImageCount;
    }

    vk::SwapchainCreateInfoKHR scci{
        {},
        _surface.get(),
        img_count,
        _swapchain_format,
        color,
        _swapchain_extent,
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

    _swapchain = _device->createSwapchainKHRUnique(scci);
    _swapchain_images = _device->getSwapchainImagesKHR(_swapchain.get());

    _swapchain_image_views.clear();
    for (auto img : _swapchain_images) {
        vk::ImageViewCreateInfo ivci{};
        ivci.setImage(img)
            .setViewType(vk::ImageViewType::e2D)
            .setFormat(_swapchain_format)
            .setSubresourceRange({vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1});
        _swapchain_image_views.push_back(_device->createImageViewUnique(ivci));
    }
}

void Engine::load_meshes() {
    _meshes.emplace_back(
        Mesh::load_obj(_allocator, "../assets/monkey_smooth.obj")
    );
    for (auto& mesh : _meshes) {
        mesh.upload();
    }
}

void Engine::init_commands() {
    spdlog::trace("Initializing command buffers");

    vk::CommandPoolCreateInfo cpci{
        vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
        _queue_family.graphics,
    };
    _cmd_pool = _device->createCommandPoolUnique(cpci);

    vk::CommandBufferAllocateInfo cbai{};
    cbai.setCommandPool(_cmd_pool.get())
        .setCommandBufferCount(1)
        .setLevel(vk::CommandBufferLevel::ePrimary);
    auto buffers = _device->allocateCommandBuffersUnique(cbai);
    if (buffers.empty()) {
        PANIC("Failed to create command buffer");
    }
    _cmd_buffer = std::move(buffers[0]);
}

void Engine::init_renderpass() {
    spdlog::trace("Initializing renderpass");

    vk::AttachmentDescription color_ad{
        {},
        _swapchain_format,
        vk::SampleCountFlagBits::e1,
        vk::AttachmentLoadOp::eClear,
        vk::AttachmentStoreOp::eStore,
        vk::AttachmentLoadOp::eDontCare,
        vk::AttachmentStoreOp::eDontCare,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::ePresentSrcKHR,
    };
    vk::AttachmentReference color_ar{0, vk::ImageLayout::eAttachmentOptimal};
    vk::SubpassDescription subpass{};
    subpass.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
        .setPColorAttachments(&color_ar)
        .setColorAttachmentCount(1);

    vk::RenderPassCreateInfo rpci{};
    rpci.setPAttachments(&color_ad)
        .setAttachmentCount(1)
        .setPSubpasses(&subpass)
        .setSubpassCount(1);

    _render_pass = _device->createRenderPassUnique(rpci);
}

void Engine::create_framebuffers() {
    spdlog::trace("Creating framebuffers");

    _framebuffers.clear();
    for (const auto& iv : _swapchain_image_views) {
        auto attachments = std::vector<vk::ImageView>{iv.get()};

        vk::FramebufferCreateInfo info{};
        info.setRenderPass(_render_pass.get())
            .setAttachments(attachments)
            .setWidth(_swapchain_extent.width)
            .setHeight(_swapchain_extent.height)
            .setLayers(1);

        _framebuffers.push_back(_device->createFramebufferUnique(info));
    }
}

void Engine::init_sync_obj() {
    spdlog::trace("Creating synchronization structures");

    _render_fence = _device->createFenceUnique(vk::FenceCreateInfo{
        vk::FenceCreateFlagBits::eSignaled});

    auto info = vk::SemaphoreCreateInfo{};
    _present_semaphore = _device->createSemaphoreUnique(info);
    _render_semaphore = _device->createSemaphoreUnique(info);
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
            .set_extent(_swapchain_extent)
            .set_cull_mode(vk::CullModeFlagBits::eBack)
            .new_pipeline()
            .add_vertex_shader(
                _shaders.vertex["mesh"].shader_module(_device.get())
            )
            .add_fragment_shader(
                _shaders.fragment["wireframe"].shader_module(_device.get())
            )
            .set_extent(_swapchain_extent)
            .set_polygon_mode(vk::PolygonMode::eLine)
            .set_cull_mode(vk::CullModeFlagBits::eNone)
            .build(_device.get(), _render_pass.get());
}

void Engine::recreate_swapchain() {
    // see:
    //   - https://stackoverflow.com/questions/59825832
    //   - https://github.com/KhronosGroup/Vulkan-Docs/issues/1059
    _device->resetFences(_render_fence.get());
    vk::PipelineStageFlags mask = vk::PipelineStageFlagBits::eBottomOfPipe;
    vk::SubmitInfo info{};
    info.setWaitSemaphores(_present_semaphore.get());
    info.setWaitDstStageMask(mask);
    _graphics_queue.submit(info, _render_fence.get());
    _graphics_queue.waitIdle();

    _device->waitIdle();

    // HACK: spin lock minimized state
    // this doesn't allow anything else in the application to execute
    i32 w;
    i32 h;
    glfwGetFramebufferSize(_window, &w, &h);
    while (w == 0 || h == 0) {
        glfwGetFramebufferSize(_window, &w, &h);
        glfwPollEvents();
    }

    _camera.set_aspect(aspect_ratio());

    destroy_swapchain();
    create_swapchain();
    create_framebuffers();
    create_pipelines();
}

void Engine::destroy_swapchain() {
    _framebuffers.clear();
    _swapchain_image_views.clear();
    _swapchain_images.clear();
    _swapchain = {};
}

u32 Engine::find_memory_type(u32 filter, vk::MemoryPropertyFlags properties) {
    auto mem_props = _gpu.getMemoryProperties();

    for (u32 i = 0; i < mem_props.memoryTypeCount; i++) {
        // NOLINTNEXTLINE(readability-implicit-bool-conversion)
        if ((filter & (1 << i)) && (mem_props.memoryTypes[i].propertyFlags &
                                    properties) == properties) {
            return i;
        }
    }

    PANIC("Failed to locate suitable memory allocation type");
}

}  // namespace hc
