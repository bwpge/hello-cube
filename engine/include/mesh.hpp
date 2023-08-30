#pragma once

#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

#include "core.hpp"
#include "types.hpp"
#include "allocator.hpp"

namespace hc {

struct Transform {
    glm::vec3 translation{0.0f};
    glm::vec3 rotation{0.0f};
    glm::vec3 scale{1.0f};
};

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
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
            offsetof(Vertex, position),
        };
        auto normal_attr = vk::VertexInputAttributeDescription{
            1,
            0,
            vk::Format::eR32G32B32Sfloat,
            offsetof(Vertex, normal),
        };
        auto color_attr = vk::VertexInputAttributeDescription{
            2,
            0,
            vk::Format::eR32G32B32Sfloat,
            offsetof(Vertex, color),
        };

        return {
            pos_attr,
            normal_attr,
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

    static Mesh quad(VmaAllocator allocator, glm::vec3 color) {
        auto mesh = Mesh{allocator};
        mesh._vertices = {
            {{-0.5f, -0.5f, 0.0f}, {0.f, 0.f, 1.f}, color},
            {{0.5f, -0.5f, 0.0f}, {0.f, 0.f, 1.f}, color},
            {{0.5f, 0.5f, 0.0f}, {0.f, 0.f, 1.f}, color},
            {{-0.5f, 0.5f, 0.0f}, {0.f, 0.f, 1.f}, color},
        };
        mesh._indices = {0, 1, 2, 2, 3, 0};

        return mesh;
    }

    static Mesh cube(VmaAllocator allocator, glm::vec3 color) {
        auto mesh = Mesh{allocator};

        // TODO(bwpge): fix cube normals
        mesh._vertices = {
            // -X side
            {{-0.5f, -0.5f, -0.5f}, {}, color},
            {{-0.5f, -0.5f, 0.5f}, {}, color},
            {{-0.5f, 0.5f, 0.5f}, {}, color},
            {{-0.5f, 0.5f, 0.5f}, {}, color},
            {{-0.5f, 0.5f, -0.5f}, {}, color},
            {{-0.5f, -0.5f, -0.5f}, {}, color},

            // -Z side
            {{-0.5f, -0.5f, -0.5f}, {}, color},
            {{0.5f, 0.5f, -0.5f}, {}, color},
            {{0.5f, -0.5f, -0.5f}, {}, color},
            {{-0.5f, -0.5f, -0.5f}, {}, color},
            {{-0.5f, 0.5f, -0.5f}, {}, color},
            {{0.5f, 0.5f, -0.5f}, {}, color},

            // -Y side
            {{-0.5f, -0.5f, -0.5f}, {}, color},
            {{0.5f, -0.5f, -0.5f}, {}, color},
            {{0.5f, -0.5f, 0.5f}, {}, color},
            {{-0.5f, -0.5f, -0.5f}, {}, color},
            {{0.5f, -0.5f, 0.5f}, {}, color},
            {{-0.5f, -0.5f, 0.5f}, {}, color},

            // +Y side
            {{-0.5f, 0.5f, -0.5f}, {}, color},
            {{-0.5f, 0.5f, 0.5f}, {}, color},
            {{0.5f, 0.5f, 0.5f}, {}, color},
            {{-0.5f, 0.5f, -0.5f}, {}, color},
            {{0.5f, 0.5f, 0.5f}, {}, color},
            {{0.5f, 0.5f, -0.5f}, {}, color},

            // +X side
            {{0.5f, 0.5f, -0.5f}, {}, color},
            {{0.5f, 0.5f, 0.5f}, {}, color},
            {{0.5f, -0.5f, 0.5f}, {}, color},
            {{0.5f, -0.5f, 0.5f}, {}, color},
            {{0.5f, -0.5f, -0.5f}, {}, color},
            {{0.5f, 0.5f, -0.5f}, {}, color},

            // +Z side
            {{-0.5f, 0.5f, 0.5f}, {}, color},
            {{-0.5f, -0.5f, 0.5f}, {}, color},
            {{0.5f, 0.5f, 0.5f}, {}, color},
            {{-0.5f, -0.5f, 0.5f}, {}, color},
            {{0.5f, -0.5f, 0.5f}, {}, color},
            {{0.5f, 0.5f, 0.5f}, {}, color},
        };
        return mesh;
    }

    [[nodiscard]]
    glm::mat4 get_transform() const;
    void upload();
    void update(double dt);
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

    Transform _transform{};
    std::vector<Vertex> _vertices{};
    std::vector<u32> _indices{};

    AllocatedBuffer _vertex_buffer{};
    AllocatedBuffer _index_buffer{};
    VmaAllocator _allocator{};
};

}  // namespace hc
