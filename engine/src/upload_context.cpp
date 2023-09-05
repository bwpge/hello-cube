#include "upload_context.hpp"

namespace hc {

void copy_staged_impl(
    vk::CommandBuffer& cmd,
    vk::Buffer& src,
    vk::Buffer& dst,
    vk::DeviceSize size
) {
    vk::BufferCopy copy{};
    copy.setSize(size);
    cmd.copyBuffer(src, dst, copy);
}

UploadContext::UploadContext(const vk::Device& device, u32 queue) {
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
    HC_ASSERT(
        buffers.size() == 1, "Should have allocated exactly one command buffer"
    );
    _cmd = std::move(buffers[0]);
}

void UploadContext::oneshot(
    vk::Device& device,
    vk::Queue& queue,
    std::function<void(vk::UniqueCommandBuffer&)>&& op
) {
    _cmd->begin(vk::CommandBufferBeginInfo{
        vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
    op(_cmd);
    _cmd->end();

    vk::SubmitInfo submit_info{};
    submit_info.setCommandBuffers(_cmd.get());
    queue.submit(submit_info, _fence.get());

    VKHPP_CHECK(
        device.waitForFences(_fence.get(), VK_TRUE, 999999999),
        "Timed out waiting for upload context fence"
    );
    device.resetFences(_fence.get());
    device.resetCommandPool(_pool.get());
}

void UploadContext::copy_staged(
    vk::Device& device,
    vk::Queue& queue,
    AllocatedBuffer& src,
    AllocatedBuffer& dst,
    vk::DeviceSize size
) {
    oneshot(device, queue, [=](vk::UniqueCommandBuffer& cmd) {
        vk::BufferCopy copy{};
        copy.setSize(size);
        cmd->copyBuffer(src.buffer, dst.buffer, copy);
    });
}

}  // namespace hc
