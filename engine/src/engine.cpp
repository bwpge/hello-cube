// vulkan needs to be included before glfw
#include "engine.hpp"
#include "logger.hpp"
#include <GLFW/glfw3.h>

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
        glfwCreateWindow(_width, _height, "Hello Triangle", nullptr, nullptr);

    if (!_window) {
        PANIC("Failed to create window");
    }

    // NOLINTBEGIN(bugprone-easily-swappable-parameters)
    glfwSetKeyCallback(
        _window,
        [](GLFWwindow* window, i32 key, i32, i32 action, i32) {
            if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
                glfwSetWindowShouldClose(window, GLFW_TRUE);
            }
        }
    );
    // NOLINTEND(bugprone-easily-swappable-parameters)

    init_vulkan();

    _is_init = true;
}

void Engine::run() {
    spdlog::info("Entering main application loop");
    while (!glfwWindowShouldClose(_window)) {
        glfwPollEvents();
        render();
    }

    _device->waitIdle();
}

void Engine::render() {
    if (_device->waitForFences(_render_fence.get(), VK_TRUE, 1000000000) !=
        vk::Result::eSuccess) {
        PANIC("Failed to wait for render fence");
    }
    _device->resetFences(_render_fence.get());

    auto res = _device->acquireNextImageKHR(
        _swapchain.get(), 1000000000, _present_semaphore.get(), nullptr
    );
    if (res.result != vk::Result::eSuccess) {
        PANIC("Failed to acquire next swapchain image");
    }
    u32 idx = res.value;

    _cmd_buffer->reset();
    _cmd_buffer->begin(vk::CommandBufferBeginInfo{
        vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
    vk::ClearValue clear{vk::ClearColorValue{0.0F, 0.0F, 0.2F, 1.0F}};
    vk::RenderPassBeginInfo rpinfo{
        _render_pass.get(),
        _framebuffers[idx].get(),
        vk::Rect2D{{0, 0}, _swapchain_extent},
        clear,
    };
    _cmd_buffer->beginRenderPass(rpinfo, vk::SubpassContents::eInline);
    _cmd_buffer->endRenderPass();
    _cmd_buffer->end();

    vk::PipelineStageFlags flags =
        vk::PipelineStageFlagBits::eColorAttachmentOutput;
    vk::SubmitInfo submit{
        _present_semaphore.get(),
        flags,
        _cmd_buffer.get(),
        _render_semaphore.get()};
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

    if (_window) {
        glfwDestroyWindow(_window);
    }
    glfwTerminate();
}

void Engine::init_vulkan() {
    spdlog::trace("Initializing Vulkan");

    create_instance();
    create_surface();
    create_device();
    init_swapchain();
    init_commands();
    init_renderpass();
    init_framebuffers();
    init_sync_obj();
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
    _indices.graphics = idx_graphics.value();
    _indices.present = idx_present.value();
    spdlog::debug(
        "Located queue indices:\n    - Graphics: {}\n    - Present: {}",
        _indices.graphics,
        _indices.present
    );

    f32 priority = 1.0F;
    auto unique_idx = std::set<u32>{_indices.graphics, _indices.present};
    auto infos = std::vector<vk::DeviceQueueCreateInfo>{};

    for (u32 idx : unique_idx) {
        vk::DeviceQueueCreateInfo dqci{{}, idx, 1, &priority, nullptr};
        infos.push_back(dqci);
    }

    // https://stackoverflow.com/questions/73746051/vulkan-how-to-enable-synchronization-2-feature
    // need to enable synchronization2 feature to use
    // VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL
    vk::PhysicalDeviceSynchronization2Features pdsf{};
    pdsf.setSynchronization2(VK_TRUE);

    vk::DeviceCreateInfo dci{};
    auto extensions = std::vector{VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    dci.setQueueCreateInfos(infos)
        .setPEnabledExtensionNames(extensions)
        .setPNext(&pdsf);

    _device = _gpu.createDeviceUnique(dci);
    _graphics_queue = _device->getQueue(_indices.graphics, 0);
}

void Engine::init_swapchain() {
    spdlog::trace("Initializing swapchain");

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
    if (_indices.graphics != _indices.present) {
        auto indices = std::vector{_indices.graphics, _indices.present};
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

    _swapchain_image_views.reserve(_swapchain_images.size());
    for (auto img : _swapchain_images) {
        vk::ImageViewCreateInfo ivci{};
        ivci.setImage(img)
            .setViewType(vk::ImageViewType::e2D)
            .setFormat(_swapchain_format)
            .setSubresourceRange({vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1});
        _swapchain_image_views.push_back(_device->createImageViewUnique(ivci));
    }
}

void Engine::init_commands() {
    spdlog::trace("Initializing command buffers");

    vk::CommandPoolCreateInfo cpci{
        vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
        _indices.graphics,
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

void Engine::init_framebuffers() {
    spdlog::trace("Initializing framebuffers");

    _framebuffers.reserve(_swapchain_image_views.size());
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

}  // namespace hc
