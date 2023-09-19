#pragma once

#include <vulkan/vulkan.hpp>
#include <glfw/glfw3.h>

#include "hvk/allocator.hpp"
#include "hvk/core.hpp"
#include "hvk/debug_utils.hpp"

namespace hvk {

struct QueueFamilyIndex {
    u32 graphics{};
    u32 present{};
    u32 transfer{};
};

struct Swapchain {
    vk::Format format{};
    vk::Extent2D extent{};
    vk::UniqueSwapchainKHR handle{};
    std::vector<vk::Image> images{};
    std::vector<vk::UniqueImageView> image_views{};
};

class VulkanContext final {
public:
    VulkanContext(const VulkanContext&) = delete;
    VulkanContext(VulkanContext&&) = delete;
    void operator=(const VulkanContext&) = delete;
    VulkanContext& operator=(VulkanContext&&) = delete;
    ~VulkanContext() = default;

    static VulkanContext& instance() {
        static VulkanContext instance{};
        return instance;
    }

    static void init(
        GLFWwindow* window,
        vk::ApplicationInfo app_info,
        const std::vector<const char*>& extensions = {}
    ) {
        spdlog::trace("Initializing Vulkan context");
        auto& ctx = instance();
        if (ctx._is_init) {
            PANIC("Cannot re-initialize Vulkan context");
        }

        // store api version for use by allocator
        if (!app_info.apiVersion) {
            spdlog::warn("Vulkan API version not specified, defaulting to 1.0");
            ctx._api_version = VK_API_VERSION_1_0;
        } else {
            ctx._api_version = app_info.apiVersion;
        }

        ctx.create_instance(app_info, extensions);
        ctx.create_surface(window);
        ctx.create_device();
        ctx.create_allocator();
        ctx.build_swapchain(window);

        ctx._is_init = true;
    }

    [[nodiscard]]
    static const vk::Instance& vk_instance() {
        return instance()._instance.get();
    }

    [[nodiscard]]
    static const vk::PhysicalDevice& gpu() {
        return instance()._gpu;
    }

    [[nodiscard]]
    static const vk::Device& device() {
        return instance()._device.get();
    }

    [[nodiscard]]
    static Allocator& allocator() {
        return instance()._allocator;
    }

    [[nodiscard]]
    static const Swapchain& swapchain() {
        return instance()._swapchain;
    }

    [[nodiscard]]
    static const vk::SurfaceKHR& surface() {
        return instance()._surface.get();
    }

    [[nodiscard]]
    static float aspect() {
        const auto extent = instance()._swapchain.extent;
        if (extent.height == 0) {
            return 0.0f;
        }

        return static_cast<float>(extent.width) /
               static_cast<float>(extent.height);
    }

    [[nodiscard]]
    static const QueueFamilyIndex& queue_families() {
        return instance()._queue_family;
    }

    [[nodiscard]]
    static const vk::Queue& graphics_queue() {
        return instance()._graphics_queue;
    }

    [[nodiscard]]
    static const vk::Queue& transfer_queue() {
        return instance()._transfer_queue;
    }

    static void flush_command_buffer(
        vk::UniqueCommandBuffer& cmd,
        const vk::Queue& queue
    ) {
        cmd->end();

        vk::SubmitInfo submit_info{};
        submit_info.setCommandBuffers(cmd.get());
        const auto& device = instance()._device.get();
        const auto fence = device.createFenceUnique({});

        queue.submit(submit_info, fence.get());
        VKHPP_CHECK(
            device.waitForFences(fence.get(), VK_TRUE, SYNC_TIMEOUT),
            "Timed out waiting for command buffer fence"
        );
    }

    [[nodiscard]]
    static vk::DescriptorSet allocate_descriptor_set(
        const vk::UniqueDescriptorPool& pool,
        const vk::UniqueDescriptorSetLayout& layout
    ) {
        vk::DescriptorSetAllocateInfo alloc_info{};
        alloc_info.setDescriptorPool(pool.get()).setSetLayouts(layout.get());

        auto sets = instance()._device->allocateDescriptorSets(alloc_info);
        HVK_ASSERT(
            sets.size() == 1, "Allocation should create one descriptor set"
        );

        return sets[0];
    }

    void build_swapchain(GLFWwindow* window);

private:
    void create_instance(
        vk::ApplicationInfo app_info,
        const std::vector<const char*>& extensions
    );
    void create_surface(GLFWwindow* window);
    void select_queue_families();
    void create_device();
    void create_allocator();

    VulkanContext() = default;

    bool _is_init{};
    u32 _api_version{VK_API_VERSION_1_3};
    vk::UniqueInstance _instance{};
    vk::PhysicalDevice _gpu{};
    vk::UniqueDevice _device{};
    vk::UniqueDebugUtilsMessengerEXT _messenger{};
    vk::UniqueSurfaceKHR _surface{};
    QueueFamilyIndex _queue_family{};
    vk::Queue _graphics_queue{};
    vk::Queue _transfer_queue{};
    Swapchain _swapchain{};
    Allocator _allocator{};
};

}  // namespace hvk
