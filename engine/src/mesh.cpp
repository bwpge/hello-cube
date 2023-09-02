#define TINYOBJLOADER_IMPLEMENTATION
#include "mesh.hpp"

namespace hc {

Mesh::Mesh(VmaAllocator allocator) : _allocator{allocator} {
    if (!_allocator) {
        PANIC("Attempted to create Mesh with null allocator handle");
    }
}

glm::mat4 Mesh::get_transform() const {
    auto translate = glm::translate(glm::mat4(1.0f), _transform.translation);
    auto rotate = glm::toMat4(glm::quat(_transform.rotation));
    auto scale = glm::scale(glm::mat4(1.0f), _transform.scale);
    return translate * rotate * scale;
}

void Mesh::upload() {
    upload_vertex_buffer();
    if (!_indices.empty()) {
        upload_index_buffer();
    }
}

void Mesh::bind(vk::CommandBuffer& cmd) const {
    if (!_vertex_buffer.buffer) {
        PANIC(spdlog::fmt_lib::format(
            "Attempted to bind mesh vertex buffer with nullptr handle",
            spdlog::fmt_lib::ptr(_vertex_buffer.buffer)
        ));
    }
    vk::Buffer vb{_vertex_buffer.buffer};
    cmd.bindVertexBuffers(0, vb, {0});

    if (!_indices.empty()) {
        if (!_index_buffer.buffer) {
            PANIC(spdlog::fmt_lib::format(
                "Attempted to bind mesh index buffer with nullptr handle",
                spdlog::fmt_lib::ptr(_index_buffer.buffer)
            ));
        }
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
    destroy_buffer(_vertex_buffer);
    destroy_buffer(_index_buffer);
}

void Mesh::set_scale(float scale) {
    _transform.scale = glm::vec3{scale};
}

void Mesh::set_rotation(glm::vec3 rotation) {
    _transform.rotation = rotation;
}

void Mesh::set_translation(glm::vec3 position) {
    _transform.translation = position;
}

void Mesh::destroy_buffer(AllocatedBuffer& buf) {
    if (buf.buffer || buf.allocation) {
        vmaDestroyBuffer(_allocator, buf.buffer, buf.allocation);
    }
}

void Mesh::upload_vertex_buffer() {
    if (_vertices.empty()) {
        spdlog::error("Calling Mesh::upload without vertex data is not valid");
        return;
    }
    create_and_upload_buffer(
        _vertices, vk::BufferUsageFlagBits::eVertexBuffer, _vertex_buffer
    );
}

void Mesh::upload_index_buffer() {
    if (_indices.empty()) {
        spdlog::error("Calling Mesh::upload without index data is not valid");
        return;
    }
    create_and_upload_buffer(
        _indices, vk::BufferUsageFlagBits::eIndexBuffer, _index_buffer
    );
}

}  // namespace hc
