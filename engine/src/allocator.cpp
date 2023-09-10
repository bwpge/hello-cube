#define VMA_IMPLEMENTATION
#include "hvk/allocator.hpp"

namespace hvk {

Allocator::Allocator(
    const vk::Instance& instance,
    const vk::PhysicalDevice& gpu,
    const vk::Device& device,
    u32 api_version
) {
    VmaAllocatorCreateInfo info{};
    info.physicalDevice = gpu;
    info.device = device;
    info.instance = instance;
    info.vulkanApiVersion = api_version;
    vmaCreateAllocator(&info, &_allocator);

    HVK_ASSERT(_allocator, "VMA failed to create allocator");
}

Allocator::Allocator(Allocator&& other) noexcept {
    if (this == &other) {
        return;
    }

    std::swap(_allocator, other._allocator);
}

Allocator& Allocator::operator=(Allocator&& rhs) noexcept {
    if (this == &rhs) {
        return *this;
    }

    std::swap(_allocator, rhs._allocator);
    return *this;
}

Allocator::~Allocator() {
    destroy_inner();
}

AllocatedBuffer Allocator::create_buffer(
    vk::DeviceSize size,
    vk::BufferUsageFlags buf_usage,
    VmaAllocationCreateFlags flags,
    VmaMemoryUsage mem_usage,
    VmaAllocationInfo* allocation_info
) {
    vk::BufferCreateInfo buf_info{};
    buf_info.setSize(size).setUsage(buf_usage);
    auto vk_buf_info = static_cast<VkBufferCreateInfo>(buf_info);

    VmaAllocationCreateInfo alloc_info{};
    alloc_info.usage = mem_usage;
    alloc_info.flags = flags;

    AllocatedBuffer buf{};
    VK_CHECK(
        vmaCreateBuffer(
            _allocator,
            &vk_buf_info,
            &alloc_info,
            &buf.buffer,
            &buf.allocation,
            allocation_info
        ),
        "Failed to create allocated buffer"
    );
    buf.size = size;

    return buf;
}

AllocatedBuffer Allocator::create_staging_buffer(vk::DeviceSize size) {
    return create_buffer(
        size,
        vk::BufferUsageFlagBits::eTransferSrc,
        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
    );
}

AllocatedImage Allocator::create_image(
    const vk::ImageCreateInfo& info,
    VmaAllocationCreateFlags flags,
    VmaMemoryUsage mem_usage
) {
    auto vk_info = static_cast<VkImageCreateInfo>(info);
    VmaAllocationCreateInfo i_alloc_info{};
    i_alloc_info.usage = mem_usage;
    i_alloc_info.flags = flags;

    AllocatedImage img{};
    VK_CHECK(
        vmaCreateImage(
            _allocator,
            &vk_info,
            &i_alloc_info,
            &img.image,
            &img.allocation,
            nullptr
        ),
        "Failed to allocate depth buffer image"
    );

    return img;
}

void Allocator::destroy_inner() {
    if (_allocator) {
        vmaDestroyAllocator(_allocator);
    }
}

}  // namespace hvk
