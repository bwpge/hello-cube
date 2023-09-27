#pragma once

#include <glfw/glfw3.h>
#include <vulkan/vulkan.hpp>

#include "hvk/allocator.hpp"
#include "hvk/core.hpp"

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

enum class QueueFamily {
    Graphics,
    Present,
    Transfer,
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
        auto& self = instance();
        if (self._is_init) {
            panic("Cannot re-initialize Vulkan context");
        }

        // store api version for use by allocator
        if (!app_info.apiVersion) {
            spdlog::warn("Vulkan API version not specified, defaulting to 1.0");
            self._api_version = VK_API_VERSION_1_0;
        } else {
            self._api_version = app_info.apiVersion;
        }

        self.create_instance(app_info, extensions);
        self.create_surface(window);
        self.create_device();
        self.create_allocator();
        self.build_swapchain(window);

        // TODO(bwpge): there should be a oneshot pool for transfer as well
        self._oneshot_pool = create_command_pool(QueueFamily::Graphics);
        self._is_init = true;
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

        return static_cast<float>(extent.width) / static_cast<float>(extent.height);
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

    [[nodiscard]]
    static vk::UniqueCommandPool create_command_pool(
        QueueFamily queue_family,
        vk::CommandPoolCreateFlags flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer
    ) {
        const auto& self = instance();

        u32 idx{};
        switch (queue_family) {
            case QueueFamily::Graphics:
                idx = self._queue_family.graphics;
                break;
            case QueueFamily::Present:
                idx = self._queue_family.present;
                break;
            case QueueFamily::Transfer:
                idx = self._queue_family.transfer;
                break;
            default:
                panic(fmt::format(
                    "Unsupported queue family type ({})",
                    static_cast<i32>(queue_family)
                ));
        }

        vk::CommandPoolCreateInfo info{flags, idx};
        return self._device->createCommandPoolUnique(info);
    }

    static vk::UniqueCommandBuffer create_command_buffer(
        const vk::UniqueCommandPool& pool,
        vk::CommandBufferLevel level = vk::CommandBufferLevel::ePrimary
    ) {
        vk::CommandBufferAllocateInfo alloc_info{};
        alloc_info.setCommandPool(pool.get()).setLevel(level).setCommandBufferCount(1);

        auto buffers = instance()._device->allocateCommandBuffersUnique(alloc_info);
        HVK_ASSERT(buffers.size() == 1, "Should have allocated exactly one command buffer");

        return std::move(buffers[0]);
    }

    static vk::UniqueCommandBuffer oneshot() {
        auto cmd = create_command_buffer(instance()._oneshot_pool);
        cmd->begin(vk::CommandBufferBeginInfo{vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
        return cmd;
    }

    static void flush_command_buffer(vk::UniqueCommandBuffer& cmd, const vk::Queue& queue) {
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
        HVK_ASSERT(sets.size() == 1, "Allocation should create one descriptor set");

        return sets[0];
    }

    static void copy_staged_buffer(
        const AllocatedBuffer& src,
        const AllocatedBuffer& dst,
        vk::DeviceSize size,
        vk::DeviceSize src_offset = {},
        vk::DeviceSize dst_offset = {}
    ) {
        auto cmd = oneshot();
        vk::BufferCopy region{src_offset, dst_offset, size};
        cmd->copyBuffer(src.buffer, dst.buffer, region);
        flush_command_buffer(cmd, transfer_queue());
    }

    void build_swapchain(GLFWwindow* window);

private:
    void create_instance(vk::ApplicationInfo app_info, const std::vector<const char*>& extensions);
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
    vk::UniqueCommandPool _oneshot_pool{};
};

}  // namespace hvk
