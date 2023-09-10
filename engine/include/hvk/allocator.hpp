#pragma once

#include <concepts>

#include <vulkan/vulkan.hpp>
#include <vk_mem_alloc.h>

#include "hvk/core.hpp"

namespace hvk {

struct AllocatedBuffer {
    VkBuffer buffer{};
    VmaAllocation allocation{};
    usize size{};
};

struct AllocatedImage {
    VkImage image{};
    VmaAllocation allocation{};
};

template <typename T>
concept IsAllocation =
    std::same_as<T, AllocatedBuffer> || std::same_as<T, AllocatedImage>;

template <typename T>
concept Allocation = requires(T t) {
    { t.allocation } -> std::same_as<VmaAllocation&>;
} && IsAllocation<T>;

class Allocator {
public:
    Allocator(
        const vk::Instance& instance,
        const vk::PhysicalDevice& gpu,
        const vk::Device& device,
        u32 api_version
    );
    Allocator() = default;
    Allocator(const Allocator&) = delete;
    Allocator(Allocator&& other) noexcept;
    Allocator& operator=(const Allocator&) = delete;
    Allocator& operator=(Allocator&& rhs) noexcept;
    ~Allocator();

    AllocatedBuffer create_buffer(
        vk::DeviceSize size,
        vk::BufferUsageFlags buf_usage,
        VmaAllocationCreateFlags flags = {},
        VmaMemoryUsage mem_usage = VMA_MEMORY_USAGE_AUTO,
        VmaAllocationInfo* allocation_info = nullptr
    );
    AllocatedBuffer create_staging_buffer(vk::DeviceSize size);
    AllocatedImage create_image(
        const vk::ImageCreateInfo& info,
        VmaAllocationCreateFlags flags = {},
        VmaMemoryUsage mem_usage = VMA_MEMORY_USAGE_AUTO
    );

    template <Allocation T>
    vk::MemoryPropertyFlags get_memory_property_flags(const T& buf) {
        VkMemoryPropertyFlags flags{};
        vmaGetAllocationMemoryProperties(_allocator, buf.allocation, &flags);
        HVK_ASSERT(
            flags != 0,
            "`vmaGetAllocationMemoryProperties` failed to get property flags"
        );
        return vk::MemoryPropertyFlags{flags};
    }

    [[nodiscard]]
    inline VmaAllocator ptr() const {
        return _allocator;
    }

    template <Allocation T>
    void copy_mapped(T& buf, void* src, usize size) {
        void* dst{};
        VK_CHECK(
            vmaMapMemory(_allocator, buf.allocation, &dst),
            "Failed to map memory allocation"
        );
        memcpy(dst, src, size);
        vmaUnmapMemory(_allocator, buf.allocation);
    };

    template <Allocation T>
    void destroy(T&) = delete;

    template <>
    void destroy(AllocatedBuffer& buf) {
        vmaDestroyBuffer(_allocator, buf.buffer, buf.allocation);
    }

    template <>
    void destroy(AllocatedImage& buf) {
        vmaDestroyImage(_allocator, buf.image, buf.allocation);
    }

private:
    void destroy_inner();

    VmaAllocator _allocator{};
};

}  // namespace hvk
