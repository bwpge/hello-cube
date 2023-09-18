#include "hvk/vk_context.hpp"

namespace hvk {

vk::Extent2D get_surface_extent(
    vk::PhysicalDevice& gpu,
    vk::SurfaceKHR& surface,
    GLFWwindow* window
) {
    const auto capabilities = gpu.getSurfaceCapabilitiesKHR(surface);

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

void VulkanContext::create_instance(
    vk::ApplicationInfo app_info,
    const std::vector<const char*>& extensions
) {
    spdlog::trace("Creating Vulkan instance");

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
        debug_callback,
    };

    auto layers = std::vector<const char*>{
        "VK_LAYER_KHRONOS_validation",
        "VK_LAYER_KHRONOS_synchronization2",
    };

    spdlog::debug(
        "Requested instance extensions: [{}]", fmt::join(extensions, ", ")
    );
    spdlog::debug("Requested instance layers: [{}]", fmt::join(layers, ", "));

    vk::InstanceCreateInfo create_info{};
    create_info.setPApplicationInfo(&app_info)
        .setPEnabledExtensionNames(extensions)
        .setPEnabledLayerNames(layers)
        .setPNext(&dmci);

    _instance = vk::createInstanceUnique(create_info);
    _messenger = _instance->createDebugUtilsMessengerEXTUnique(dmci);
}

void VulkanContext::create_surface(GLFWwindow* window) {
    spdlog::trace("Initializing window surface");

    VkSurfaceKHR surface{};
    if (glfwCreateWindowSurface(_instance.get(), window, nullptr, &surface) !=
        VK_SUCCESS) {
        PANIC("Failed to create window surface");
    }
    if (!surface) {
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

void VulkanContext::create_device() {
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

    spdlog::debug("Enumerating queue family properties...");
    auto queues = _gpu.getQueueFamilyProperties();
    auto idx_graphics = std::optional<u32>{};
    auto idx_present = std::optional<u32>();
    auto idx_transfer = std::optional<u32>();

    for (u32 i = 0; i < static_cast<u32>(queues.size()); i++) {
        const auto props = queues[i];
        const auto has_graphics =
            props.queueFlags & vk::QueueFlagBits::eGraphics;
        const auto has_present =
            _gpu.getSurfaceSupportKHR(i, _surface.get()) == VK_TRUE;
        const auto has_transfer =
            (props.queueFlags & vk::QueueFlagBits::eTransfer);

        if (has_graphics) {
            spdlog::debug("Found graphics queue: index={}", i);
            idx_graphics = i;
        }
        if (has_present) {
            spdlog::debug("Found present queue: index={}", i);
            idx_present = i;
        }
        if (has_transfer) {
            spdlog::debug("Found transfer queue: index={}", i);
            idx_transfer = i;
        }
        if (idx_graphics.has_value() && idx_present.has_value() &&
            idx_transfer.has_value()) {
            break;
        }
    }

    if (!idx_graphics.has_value() || !idx_present.has_value() ||
        !idx_transfer.has_value()) {
        PANIC("Failed to locate required queue indices");
    }
    _queue_family.graphics = idx_graphics.value();
    _queue_family.present = idx_present.value();
    _queue_family.transfer = idx_transfer.value();

    f32 priority = 1.0f;
    auto unique_idx = std::set<u32>{
        _queue_family.graphics, _queue_family.present, _queue_family.transfer};
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

void VulkanContext::create_allocator() {
    _allocator = Allocator{_instance.get(), _gpu, _device.get(), _api_version};
}

void VulkanContext::build_swapchain(GLFWwindow* window) {
    spdlog::trace("Creating swapchain");
    _swapchain = {};

    const auto capabilities = _gpu.getSurfaceCapabilitiesKHR(_surface.get());
    _swapchain.extent = get_surface_extent(_gpu, _surface.get(), window);
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
}

}  // namespace hvk
