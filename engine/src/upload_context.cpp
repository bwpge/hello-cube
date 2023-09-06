#include "upload_context.hpp"

namespace hvk {

UploadContext::UploadContext(u32 queue) {
    const auto& device = VulkanContext::device();
    _fence = device.createFenceUnique({});

    vk::CommandPoolCreateInfo pool_info{
        vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
        queue,
    };
    _pool = device.createCommandPoolUnique(pool_info);

    vk::CommandBufferAllocateInfo alloc_info{};
    alloc_info.setCommandPool(_pool.get())
        .setCommandBufferCount(1)
        .setLevel(vk::CommandBufferLevel::ePrimary);
    auto buffers = device.allocateCommandBuffersUnique(alloc_info);
    HVK_ASSERT(
        buffers.size() == 1, "Should have allocated exactly one command buffer"
    );
    _cmd = std::move(buffers[0]);
}

void UploadContext::oneshot(
    const vk::Queue& queue,
    std::function<void(vk::UniqueCommandBuffer&)>&& op
) {
    const auto& device = VulkanContext::device();

    _cmd->begin(vk::CommandBufferBeginInfo{
        vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
    op(_cmd);
    _cmd->end();

    vk::SubmitInfo submit_info{};
    submit_info.setCommandBuffers(_cmd.get());
    queue.submit(submit_info, _fence.get());

    VKHPP_CHECK(
        device.waitForFences(_fence.get(), VK_TRUE, SYNC_TIMEOUT),
        "Timed out waiting for upload fence"
    );
    device.resetFences(_fence.get());
    device.resetCommandPool(_pool.get());
}

void UploadContext::copy_staged(
    const vk::Queue& queue,
    AllocatedBuffer& src,
    AllocatedBuffer& dst,
    vk::DeviceSize size
) {
    oneshot(queue, [=](vk::UniqueCommandBuffer& cmd) {
        vk::BufferCopy region{};
        region.setSize(size);
        cmd->copyBuffer(src.buffer, dst.buffer, region);
    });
}

}  // namespace hvk
