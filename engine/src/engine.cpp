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

void log_list(std::string_view caption, const std::vector<const char*>& list) {
    std::ostringstream msg{};
    msg << caption << "\n";

    if (list.empty()) {
        msg << "    - <none>";
    } else {
        i32 len = static_cast<i32>(list.size());
        for (i32 i = 0; i < len; i++) {
            msg << "    - " << list[i];
            if (i + 1 != len) {
                msg << "\n";
            }
        }
    }

    spdlog::debug(msg.str());
}

void log_devices(const std::vector<vk::PhysicalDevice>& list) {
    std::ostringstream msg{};
    msg << "Physical devices found:"
        << "\n";

    if (list.empty()) {
        msg << "    - <none>";
    } else {
        i32 len = static_cast<i32>(list.size());
        for (i32 i = 0; i < len; i++) {
            const auto p = list[i].getProperties();
            msg << "    - " << p.deviceName << " (id=" << p.deviceID << ")";
            if (i + 1 != len) {
                msg << "\n";
            }
        }
    }

    spdlog::debug(msg.str());
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
        spdlog::critical("Failed to create window, exiting");
        std::exit(-1);
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
}

void Engine::render() {
    // TODO(bwpge)
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

    // create vulkan instance
    auto extensions = get_extensions();

    log_list("Requested instance extensions:", extensions);

    auto layers = std::vector<const char*>{
        "VK_LAYER_KHRONOS_validation", "VK_LAYER_KHRONOS_synchronization2"};
    log_list("Requested validation layers:", layers);

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
        spdlog::critical("Failed to create window surface");
        abort();
    }
    if (surface == VK_NULL_HANDLE) {
        spdlog::critical("Failed to create window surface");
        abort();
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
    log_devices(devices);

    i32 selected = -1;
    for (i32 i = 0; i < static_cast<i32>(devices.size()); i++) {
        const auto p = devices[i].getProperties();
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
        spdlog::critical("Failed to locate a suitable device");
        abort();
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
        spdlog::critical("Failed to locate required queue indices");
        abort();
    }
    _queue_indices.graphics = idx_graphics.value();
    _queue_indices.present = idx_present.value();
    spdlog::debug(
        "Located queue indices:\n    - Graphics: {}\n    - Present: {}",
        _queue_indices.graphics,
        _queue_indices.present
    );

    f32 priority = 1.0F;
    auto unique_idx = std::set<u32>{idx_graphics.value(), idx_present.value()};
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
}

void Engine::init_swapchain() {
    spdlog::trace("Initializing swapchain");

    const auto capabilities = _gpu.getSurfaceCapabilitiesKHR(_surface.get());
    const auto formats = _gpu.getSurfaceFormatsKHR(_surface.get());
    const auto modes = _gpu.getSurfacePresentModesKHR(_surface.get());

    vk::Extent2D extent{};
    if (capabilities.currentExtent.width != std::numeric_limits<u32>::max()) {
        extent = capabilities.currentExtent;
    } else {
        i32 width{};
        i32 height{};
        glfwGetFramebufferSize(_window, &width, &height);
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

    auto format = std::optional<vk::Format>{};
    auto color = std::optional<vk::ColorSpaceKHR>{};
    for (usize i = 0; i < formats.size(); i++) {}
    for (const auto& f : formats) {
        if (f.format == vk::Format::eB8G8R8A8Unorm) {
            format = f.format;
        }
        if (f.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
            color = f.colorSpace;
        }
        if (format.has_value() && color.has_value()) {
            break;
        }
    }
    if (!format.has_value()) {
        spdlog::critical("GPU does not have a suitable image format");
        abort();
    } else if (!color.has_value()) {
        spdlog::critical("GPU does not have a suitable image color space");
        abort();
    }
    _swapchain_format = format.value();

    auto mode = std::optional<vk::PresentModeKHR>{};
    for (const auto& m : modes) {
        if (m == vk::PresentModeKHR::eFifo) {
            mode = m;
        }
    }
    if (!mode.has_value()) {
        spdlog::critical("GPU does not have a suitable present mode");
        abort();
    }

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
        color.value(),
        extent,
        1,
        vk::ImageUsageFlagBits::eColorAttachment,
    };
    if (_queue_indices.graphics != _queue_indices.present) {
        auto indices =
            std::vector{_queue_indices.graphics, _queue_indices.present};
        scci.setImageSharingMode(vk::SharingMode::eConcurrent)
            .setQueueFamilyIndices(indices);
    } else {
        scci.setImageSharingMode(vk::SharingMode::eExclusive);
    }
    scci.setPreTransform(capabilities.currentTransform)
        .setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque)
        .setPresentMode(mode.value())
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
        _queue_indices.graphics,
    };
    _cmd_pool = _device->createCommandPoolUnique(cpci);

    vk::CommandBufferAllocateInfo cbai{};
    cbai.setCommandPool(_cmd_pool.get())
        .setCommandBufferCount(1)
        .setLevel(vk::CommandBufferLevel::ePrimary);
    _cmd_buffer = std::move(_device->allocateCommandBuffersUnique(cbai)[0]);
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

void Engine::init_framebuffers() {  // NOLINT
    spdlog::trace("Initializing framebuffers");

    // TODO(bwpge)
}

}  // namespace hc
