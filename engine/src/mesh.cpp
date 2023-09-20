#define TINYOBJLOADER_IMPLEMENTATION
#include "hvk/mesh.hpp"

namespace hvk {

Mesh::~Mesh() {
    destroy();
}

void Mesh::upload(const vk::Queue& queue, UploadContext& ctx) {
    HVK_ASSERT(!_vertices.empty(), "Cannot upload mesh without vertex data");

    create_and_upload_buffer(
        queue,
        ctx,
        _vertices,
        vk::BufferUsageFlagBits::eVertexBuffer,
        _vertex_buffer
    );
    if (!_indices.empty()) {
        create_and_upload_buffer(
            queue,
            ctx,
            _indices,
            vk::BufferUsageFlagBits::eIndexBuffer,
            _index_buffer
        );
    }
}

void Mesh::bind(const vk::UniqueCommandBuffer& cmd) const {
    bind(cmd.get());
}

void Mesh::bind(const vk::CommandBuffer& cmd) const {
    HVK_ASSERT(_vertex_buffer.buffer, "Cannot bind mesh vertex buffer with null handle");
    vk::Buffer vb{_vertex_buffer.buffer};
    cmd.bindVertexBuffers(0, vb, {0});

    if (!_indices.empty()) {
        HVK_ASSERT(_index_buffer.buffer, "Cannot bind mesh index buffer with null handle");
        vk::Buffer ib{_index_buffer.buffer};
        cmd.bindIndexBuffer(ib, 0, vk::IndexType::eUint32);
    }
}

void Mesh::draw(const vk::UniqueCommandBuffer& cmd) const {
    draw(cmd.get());
}

void Mesh::draw(const vk::CommandBuffer& cmd) const {
    if (_indices.empty()) {
        cmd.draw(static_cast<u32>(_vertices.size()), 1, 0, 0);
    } else {
        cmd.drawIndexed(static_cast<u32>(_indices.size()), 1, 0, 0, 0);
    }
}

void Mesh::destroy() {
    auto& allocator = VulkanContext::allocator();
    allocator.destroy(_vertex_buffer);
    allocator.destroy(_index_buffer);
}

}  // namespace hvk
