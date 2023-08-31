#pragma once

#include <filesystem>

#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <tiny_obj_loader.h>

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

    static Mesh load_obj(
        VmaAllocator allocator,
        const std::filesystem::path& path
    ) {
        std::ifstream file{path, std::ios::in | std::ios::binary};
        if (!file.is_open()) {
            PANIC(spdlog::fmt_lib::format(
                "failed to open obj file '{}'", path.string()
            ));
        }

        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;

        std::string warn;
        std::string err;

        tinyobj::LoadObj(
            &attrib,
            &shapes,
            &materials,
            &warn,
            &err,
            path.string().c_str(),
            nullptr
        );
        if (!warn.empty()) {
            spdlog::warn("[tiny_obj_loader] {}", warn);
        }
        if (!err.empty()) {
            PANIC(spdlog::fmt_lib::format("[tiny_obj_loader] {}", err));
        }

        Mesh mesh{};
        mesh._allocator = allocator;

        for (auto& shape : shapes) {
            size_t offset = 0;
            for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); f++) {
                // hardcode loading triangles
                usize fv = 3;
                for (size_t v = 0; v < fv; v++) {
                    tinyobj::index_t idx = shape.mesh.indices[offset + v];
                    Vertex vertex;

                    vertex.position = {
                        attrib.vertices[3 * idx.vertex_index + 0],
                        attrib.vertices[3 * idx.vertex_index + 1],
                        attrib.vertices[3 * idx.vertex_index + 2],
                    };
                    vertex.normal = {
                        attrib.normals[3 * idx.normal_index + 0],
                        attrib.normals[3 * idx.normal_index + 1],
                        attrib.normals[3 * idx.normal_index + 2],
                    };
                    // DEBUG: set color to normal
                    vertex.color = vertex.normal;

                    mesh._vertices.push_back(vertex);
                }
                offset += fv;
            }
        }

        return mesh;
    }

    [[nodiscard]]
    glm::mat4 get_transform() const;
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

    Transform _transform{};
    std::vector<Vertex> _vertices{};
    std::vector<u32> _indices{};

    AllocatedBuffer _vertex_buffer{};
    AllocatedBuffer _index_buffer{};
    VmaAllocator _allocator{};
};

}  // namespace hc
