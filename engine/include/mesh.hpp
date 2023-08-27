#pragma once

#include <glm/glm.hpp>

#include "core.hpp"
#include "types.hpp"
#include "allocator.hpp"

namespace hc {

struct Vertex {
    glm::vec3 pos;
    glm::vec3 color;

    static std::vector<vk::VertexInputBindingDescription>
    get_binding_description() {
        return {{
            0,
            sizeof(Vertex),
            vk::VertexInputRate::eVertex,
        }};
    }

    static std::vector<vk::VertexInputAttributeDescription>
    get_attr_description() {
        auto pos_attr = vk::VertexInputAttributeDescription{
            0,
            0,
            vk::Format::eR32G32B32Sfloat,
            offsetof(Vertex, pos),
        };
        auto color_attr = vk::VertexInputAttributeDescription{
            1,
            0,
            vk::Format::eR32G32B32Sfloat,
            offsetof(Vertex, color),
        };

        return {
            pos_attr,
            color_attr,
        };
    }
};

class Mesh {
public:
    Mesh() = default;
    explicit Mesh(VmaAllocator allocator);
    Mesh(const Mesh&) = delete;
    Mesh(Mesh&&) noexcept = default;
    Mesh& operator=(const Mesh&) = delete;
    Mesh& operator=(Mesh&&) noexcept = default;
    ~Mesh() = default;

    static Mesh quad(VmaAllocator allocator) {
        auto mesh = Mesh{allocator};
        mesh._vertices = {
            {{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}},
            {{0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}},
            {{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}},
            {{-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}},
        };
        mesh._indices = {0, 1, 2, 2, 3, 0};

        return mesh;
    }

    void upload();
    void bind(vk::CommandBuffer& cmd) const;
    void draw(const vk::UniqueCommandBuffer& cmd) const;
    void draw(const vk::CommandBuffer& cmd) const;
    void destroy();

private:
    void destroy_buffer(AllocatedBuffer& buffer);
    void upload_vertex_buffer();
    void upload_index_buffer();

    template <typename T>
    void create_and_upload_buffer(
        std::vector<T>& src,
        vk::BufferUsageFlags usage,
        AllocatedBuffer& buffer
    ) {
        const auto size =
            src.size() *
            sizeof(std::remove_reference<decltype(src)>::type::value_type);
        spdlog::debug(
            "Uploading Mesh buffer: size={}, usage={}",
            size,
            vk::to_string(usage)
        );

        vk::BufferCreateInfo buffer_info{};
        buffer_info.setSize(size).setUsage(usage);
        auto vk_buffer_info = static_cast<VkBufferCreateInfo>(buffer_info);

        VmaAllocationCreateInfo alloc_info{};
        alloc_info.usage = VMA_MEMORY_USAGE_AUTO;
        alloc_info.flags =
            VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

        VK_CHECK(
            vmaCreateBuffer(
                _allocator,
                &vk_buffer_info,
                &alloc_info,
                &buffer.buffer,
                &buffer.allocation,
                nullptr
            ),
            "Failed to allocate mesh buffer"
        );

        void* data;
        VK_CHECK(
            vmaMapMemory(_allocator, buffer.allocation, &data),
            "Failed to map memory allocation"
        );
        memcpy(data, src.data(), size);
        vmaUnmapMemory(_allocator, buffer.allocation);
    }

    std::vector<Vertex> _vertices{};
    std::vector<u32> _indices{0, 1, 2, 2, 3, 0};

    AllocatedBuffer _vertex_buffer{};
    AllocatedBuffer _index_buffer{};
    VmaAllocator _allocator{};
};

}  // namespace hc
