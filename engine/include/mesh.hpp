#pragma once

#include <filesystem>

#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <tiny_obj_loader.h>

#include "core.hpp"
#include "allocator.hpp"
#include "upload_context.hpp"

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

    static std::vector<vk::VertexInputBindingDescription> binding_desc() {
        return {{
            0,
            sizeof(Vertex),
            vk::VertexInputRate::eVertex,
        }};
    }

    static std::vector<vk::VertexInputAttributeDescription> attr_desc() {
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
    Mesh(const Mesh&) = delete;
    Mesh(Mesh&&) noexcept = default;
    Mesh& operator=(const Mesh&) = delete;
    Mesh& operator=(Mesh&&) noexcept = default;
    ~Mesh() = default;

    static Mesh quad(glm::vec3 color) {
        Mesh mesh{};
        mesh._vertices = {
            {{-0.5f, -0.5f, 0.0f}, {0.f, 0.f, 1.f}, color},
            {{0.5f, -0.5f, 0.0f}, {0.f, 0.f, 1.f}, color},
            {{0.5f, 0.5f, 0.0f}, {0.f, 0.f, 1.f}, color},
            {{-0.5f, 0.5f, 0.0f}, {0.f, 0.f, 1.f}, color},
        };
        mesh._indices = {0, 1, 2, 2, 3, 0};

        return mesh;
    }

    static Mesh cube(float size = 1.0f, glm::vec3 color = {1.0f, 0.0f, 1.0f}) {
        Mesh mesh{};
        const auto s = size / 2.0f;

        mesh._vertices = {
            // -X side
            {{-s, -s, -s}, {-1.0f, 0.0f, 0.0f}, color},
            {{-s, -s, s}, {-1.0f, 0.0f, 0.0f}, color},
            {{-s, s, s}, {-1.0f, 0.0f, 0.0f}, color},
            {{-s, s, s}, {-1.0f, 0.0f, 0.0f}, color},
            {{-s, s, -s}, {-1.0f, 0.0f, 0.0f}, color},
            {{-s, -s, -s}, {-1.0f, 0.0f, 0.0f}, color},

            // -Z side
            {{-s, -s, -s}, {0.0f, 0.0f, -1.0f}, color},
            {{s, s, -s}, {0.0f, 0.0f, -1.0f}, color},
            {{s, -s, -s}, {0.0f, 0.0f, -1.0f}, color},
            {{-s, -s, -s}, {0.0f, 0.0f, -1.0f}, color},
            {{-s, s, -s}, {0.0f, 0.0f, -1.0f}, color},
            {{s, s, -s}, {0.0f, 0.0f, -1.0f}, color},

            // -Y side
            {{-s, -s, -s}, {0.0f, -1.0, 0.0f}, color},
            {{s, -s, -s}, {0.0f, -1.0, 0.0f}, color},
            {{s, -s, s}, {0.0f, -1.0, 0.0f}, color},
            {{-s, -s, -s}, {0.0f, -1.0, 0.0f}, color},
            {{s, -s, s}, {0.0f, -1.0, 0.0f}, color},
            {{-s, -s, s}, {0.0f, -1.0, 0.0f}, color},

            // +Y side
            {{-s, s, -s}, {0.0f, 1.0f, 0.0f}, color},
            {{-s, s, s}, {0.0f, 1.0f, 0.0f}, color},
            {{s, s, s}, {0.0f, 1.0f, 0.0f}, color},
            {{-s, s, -s}, {0.0f, 1.0f, 0.0f}, color},
            {{s, s, s}, {0.0f, 1.0f, 0.0f}, color},
            {{s, s, -s}, {0.0f, 1.0f, 0.0f}, color},

            // +X side
            {{s, s, -s}, {1.0f, 0.0f, 0.0f}, color},
            {{s, s, s}, {1.0f, 0.0f, 0.0f}, color},
            {{s, -s, s}, {1.0f, 0.0f, 0.0f}, color},
            {{s, -s, s}, {1.0f, 0.0f, 0.0f}, color},
            {{s, -s, -s}, {1.0f, 0.0f, 0.0f}, color},
            {{s, s, -s}, {1.0f, 0.0f, 0.0f}, color},

            // +Z side
            {{-s, s, s}, {0.0f, 0.0f, 1.0f}, color},
            {{-s, -s, s}, {0.0f, 0.0f, 1.0f}, color},
            {{s, s, s}, {0.0f, 0.0f, 1.0f}, color},
            {{-s, -s, s}, {0.0f, 0.0f, 1.0f}, color},
            {{s, -s, s}, {0.0f, 0.0f, 1.0f}, color},
            {{s, s, s}, {0.0f, 0.0f, 1.0f}, color},
        };
        return mesh;
    }

    // implementation adapted from http://www.songho.ca/opengl/gl_sphere.html
    static Mesh sphere(float radius, glm::vec3 color, u32 sectors, u32 stacks) {
        Mesh mesh{};

        float d_sector = glm::two_pi<float>() / static_cast<float>(sectors);
        float d_step = glm::pi<float>() / static_cast<float>(stacks);
        for (u32 i = 0; i <= stacks; ++i) {
            auto stack_angle =
                glm::half_pi<float>() - static_cast<float>(i) * d_step;
            auto xy = radius * glm::cos(stack_angle);
            auto z = radius * glm::sin(stack_angle);

            for (u32 j = 0; j <= sectors; ++j) {
                auto sector_angle = static_cast<float>(j) * d_sector;

                auto x = xy * glm::cos(sector_angle);
                auto y = xy * glm::sin(sector_angle);

                auto pos = glm::vec3{x, y, z};
                auto normal = glm::normalize(pos);
                mesh._vertices.emplace_back(pos, normal, color);
            }
        }

        for (u32 i = 0; i < stacks; ++i) {
            auto k1 = i * (sectors + 1);
            auto k2 = k1 + sectors + 1;

            for (u32 j = 0; j < sectors; ++j, ++k1, ++k2) {
                if (i != 0) {
                    mesh._indices.push_back(k1);
                    mesh._indices.push_back(k2);
                    mesh._indices.push_back(k1 + 1);
                }

                if (i != (stacks - 1)) {
                    mesh._indices.push_back(k1 + 1);
                    mesh._indices.push_back(k2);
                    mesh._indices.push_back(k2 + 1);
                }
            }
        }

        return mesh;
    }

    static Mesh load_obj(const std::filesystem::path& path) {
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
            path.parent_path().string().c_str()
        );
        if (!warn.empty()) {
            spdlog::warn("[tiny_obj_loader] {}", warn);
        }
        if (!err.empty()) {
            PANIC(spdlog::fmt_lib::format("[tiny_obj_loader] {}", err));
        }

        Mesh mesh{};

        for (auto& shape : shapes) {
            size_t offset = 0;
            for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); f++) {
                // hardcode loading triangles
                usize fv = 3;
                for (size_t v = 0; v < fv; v++) {
                    tinyobj::index_t idx = shape.mesh.indices[offset + v];
                    Vertex vertex{};

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
    glm::mat4 transform() const;
    void upload(const vk::Queue& queue, UploadContext& ctx);
    void bind(vk::CommandBuffer& cmd) const;
    void draw(const vk::UniqueCommandBuffer& cmd) const;
    void draw(const vk::CommandBuffer& cmd) const;
    void destroy();

    void translate(glm::vec3 translation);
    void set_translation(glm::vec3 position);
    void rotate(glm::vec3 rotation);
    void set_rotation(glm::vec3 rotation);
    void scale(float scale);
    void set_scale(float scale);

private:
    template <typename T>
    void create_and_upload_buffer(
        const vk::Queue& queue,
        UploadContext& ctx,
        std::vector<T>& src,
        vk::BufferUsageFlags usage,
        AllocatedBuffer& buffer
    ) {
        auto& allocator = VulkanContext::allocator();
        const auto size =
            src.size() *
            sizeof(std::remove_reference<decltype(src)>::type::value_type);

        // stage buffer data for upload
        auto staging_buf = allocator.create_staging_buffer(size);
        allocator.copy_mapped(staging_buf, src.data(), size);

        // create gpu-side buffer
        auto gpu_buf = allocator.create_buffer(
            size, usage | vk::BufferUsageFlagBits::eTransferDst
        );

        // upload to gpu buffer
        ctx.copy_staged(queue, staging_buf, gpu_buf, size);
        allocator.destroy(staging_buf);

        // populate destination buffer
        allocator.destroy(buffer);
        std::swap(buffer, gpu_buf);
    }

    Transform _transform{};
    std::vector<Vertex> _vertices{};
    std::vector<u32> _indices{};

    AllocatedBuffer _vertex_buffer{};
    AllocatedBuffer _index_buffer{};
};

}  // namespace hc
