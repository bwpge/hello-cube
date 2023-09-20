#include "hvk/vk_context.hpp"
#include "hvk/debug_utils.hpp"

namespace hvk {

vk::Extent2D
get_surface_extent(vk::PhysicalDevice& gpu, vk::SurfaceKHR& surface, GLFWwindow* window) {
    const auto capabilities = gpu.getSurfaceCapabilitiesKHR(surface);

    vk::Extent2D extent{};
    if (capabilities.currentExtent.width != std::numeric_limits<u32>::max()) {
        extent = capabilities.currentExtent;
    } else {
        i32 width{};
        i32 height{};
        glfwGetFramebufferSize(window, &width, &height);
        extent = vk::Extent2D{static_cast<u32>(width), static_cast<u32>(height)};
    }
    extent.width = std::
        clamp(extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
    extent.height = std::clamp(
        extent.height,
        capabilities.minImageExtent.height,
        capabilities.maxImageExtent.height
    );

    return extent;
}

void VulkanContext::
    create_instance(vk::ApplicationInfo app_info, const std::vector<const char*>& extensions) {
    spdlog::trace("Creating Vulkan instance");

    auto dmci = vk::DebugUtilsMessengerCreateInfoEXT{
        {},
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose
            | vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo
            | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning
            | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
        vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral
            | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance
            | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation
            | vk::DebugUtilsMessageTypeFlagBitsEXT::eDeviceAddressBinding,
        debug_callback,
    };

    auto layers = std::vector<const char*>{
        "VK_LAYER_KHRONOS_validation",
        "VK_LAYER_KHRONOS_synchronization2",
    };

    spdlog::debug("Requested instance extensions: [{}]", fmt::join(extensions, ", "));
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
    if (glfwCreateWindowSurface(_instance.get(), window, nullptr, &surface) != VK_SUCCESS) {
        panic("Failed to create window surface");
    }
    if (!surface) {
        panic("Failed to create window surface");
    } else {
        // https://github.com/KhronosGroup/Vulkan-Hpp/blob/e2f5348e28ba22dd9b87028f2d9bb7d556aa7b6e/samples/utils/utils.cpp#L790-L798
        auto deleter = vk::ObjectDestroy<vk::Instance, VULKAN_HPP_DEFAULT_DISPATCHER_TYPE>(
            _instance.get()
        );
        _surface = vk::UniqueSurfaceKHR{surface, deleter};
    }
}

void VulkanContext::select_queue_families() {
    auto queue_family_props = _gpu.getQueueFamilyProperties();
    auto idx_graphics = std::vector<u32>{};
    auto idx_present = std::vector<u32>();
    auto idx_transfer = std::vector<u32>();

    for (u32 i = 0; i < static_cast<u32>(queue_family_props.size()); i++) {
        const auto props = queue_family_props[i];
        const auto has_graphics = props.queueFlags & vk::QueueFlagBits::eGraphics;
        const auto has_present = _gpu.getSurfaceSupportKHR(i, _surface.get()) == VK_TRUE;
        const auto has_transfer = (props.queueFlags & vk::QueueFlagBits::eTransfer);

        if (has_graphics) {
            idx_graphics.push_back(i);
        }
        if (has_present) {
            idx_present.push_back(i);
        }
        if (has_transfer) {
            idx_transfer.push_back(i);
        }
    }
    spdlog::trace(
        "Enumerated queue families by capabilities:\n"
        "    Graphics: {}\n"
        "    Present:  {}\n"
        "    Transfer: {}",
        fmt::join(idx_graphics, ", "),
        fmt::join(idx_present, ", "),
        fmt::join(idx_transfer, ", ")
    );

    if (idx_graphics.empty() || idx_present.empty() || idx_transfer.empty()) {
        panic("Failed to locate required queue indices");
    }

    _queue_family = {
        idx_graphics[0],
        idx_present[0],
        idx_transfer[0],
    };
    // try to find a different graphics/transfer queue, but transfers
    // need graphics capabilities to transition images to shader stages
    // (without a ton of boilerplate code for different transitions)
    for (const auto i : idx_transfer) {
        if (i != _queue_family.graphics && i != _queue_family.present
            && queue_family_props[i].queueFlags & vk::QueueFlagBits::eGraphics) {
            _queue_family.transfer = i;
        }
    }
    spdlog::debug(
        "Selected queue family indices:\n"
        "    Graphics: {}\n"
        "    Present:  {}\n"
        "    Transfer: {}",
        _queue_family.graphics,
        _queue_family.present,
        _queue_family.transfer
    );
}

void VulkanContext::create_device() {
    spdlog::trace("Selecting physical device");

    const auto devices = _instance->enumeratePhysicalDevices();
    spdlog::debug("Found {} {}", devices.size(), devices.size() == 1 ? "device" : "devices");

    std::optional<i32> selected{};
    for (i32 i = 0; i < static_cast<i32>(devices.size()); i++) {
        auto p = devices[i].getProperties();
        if (p.deviceType == vk::PhysicalDeviceType::eDiscreteGpu) {
            selected = i;
            spdlog::debug("Selected device '{}'", static_cast<const char*>(p.deviceName));
            selected = i;
            break;
        }
    }
    if (!selected.has_value()) {
        panic("Failed to locate a suitable device");
    }

    _gpu = devices[selected.value()];
    select_queue_families();

    auto unique_queues = std::unordered_map<u32, u32>{};
    unique_queues[_queue_family.graphics] += 1;
    unique_queues[_queue_family.present] += 1;
    unique_queues[_queue_family.transfer] += 1;

    auto priorities = std::vector<std::vector<float>>(unique_queues.size());
    for (const auto& [idx, count] : unique_queues) {
        // TODO(bwpge): provide actual queue priorities
        priorities[idx] = std::vector<float>(count, 1.0f);
    }

    auto queue_create_infos = std::vector<vk::DeviceQueueCreateInfo>{};
    for (const auto& [idx, _] : unique_queues) {
        vk::DeviceQueueCreateInfo dqci{};
        dqci.setQueueFamilyIndex(idx).setQueuePriorities(priorities[idx]);
        queue_create_infos.push_back(dqci);
    }

    // required to render wireframe (VK_POLYGON_MODE_LINE)
    vk::PhysicalDeviceFeatures features{};
    features.setFillModeNonSolid(VK_TRUE);

    // https://stackoverflow.com/questions/73746051/vulkan-how-to-enable-synchronization-2-feature
    // need to enable synchronization2 feature to use
    // VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL
    vk::PhysicalDeviceSynchronization2Features sync_features{};
    sync_features.setSynchronization2(VK_TRUE);

    // https://vulkan.lunarg.com/doc/view/1.3.250.1/windows/1.3-extensions/vkspec.html#VUID-VkAttachmentReference2-separateDepthStencilLayouts-03313
    // need to enable to use VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL in depth
    // attachment for render subpass
    vk::PhysicalDeviceVulkan12Features phys_v12_features{};
    phys_v12_features.setSeparateDepthStencilLayouts(VK_TRUE);

    vk::DeviceCreateInfo create_info{};
    auto extensions = std::vector{VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    create_info.setQueueCreateInfos(queue_create_infos)
        .setPEnabledFeatures(&features)
        .setPEnabledExtensionNames(extensions);
    vk::StructureChain create_info_chain = {
        create_info,
        sync_features,
        phys_v12_features,
    };

    _device = _gpu.createDeviceUnique(create_info_chain.get());

    u32 queue_num{};
    spdlog::
        debug("Storing graphics queue handle: family={} (#{})", _queue_family.graphics, queue_num);
    _graphics_queue = _device->getQueue(_queue_family.graphics, queue_num++);
    spdlog::
        debug("Storing transfer queue handle: family={} (#{})", _queue_family.graphics, queue_num);
    _transfer_queue = _device->getQueue(_queue_family.transfer, queue_num);
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
    if (capabilities.maxImageCount > 0 && img_count > capabilities.maxImageCount) {
        img_count = capabilities.maxImageCount;
    }

    vk::SwapchainCreateInfoKHR create_info{
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
        auto indices = std::vector{_queue_family.graphics, _queue_family.present};
        create_info.setImageSharingMode(vk::SharingMode::eConcurrent)
            .setQueueFamilyIndices(indices);
    } else {
        create_info.setImageSharingMode(vk::SharingMode::eExclusive);
    }
    create_info.setPreTransform(capabilities.currentTransform)
        .setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque)
        .setPresentMode(mode)
        .setClipped(VK_TRUE);

    _swapchain.handle = _device->createSwapchainKHRUnique(create_info);
    _swapchain.images = _device->getSwapchainImagesKHR(_swapchain.handle.get());

    _swapchain.image_views.clear();
    for (auto img : _swapchain.images) {
        vk::ImageViewCreateInfo info{};
        info.setImage(img).setViewType(vk::ImageViewType::e2D).setFormat(_swapchain.format);
        info.subresourceRange.setAspectMask(vk::ImageAspectFlagBits::eColor)
            .setLevelCount(1)
            .setLayerCount(1);
        _swapchain.image_views.push_back(_device->createImageViewUnique(info));
    }
}

}  // namespace hvk
