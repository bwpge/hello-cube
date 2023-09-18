#pragma once

#include <filesystem>

#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <tiny_obj_loader.h>

#include "hvk/core.hpp"
#include "hvk/allocator.hpp"
#include "hvk/upload_context.hpp"

namespace hvk {

struct Vertex {
    glm::vec3 position{};
    glm::vec3 normal{};
    glm::vec3 color{};
    glm::vec2 uv{};

    static std::vector<vk::VertexInputBindingDescription> binding_desc() {
        vk::VertexInputBindingDescription desc{
            0,
            sizeof(Vertex),
            vk::VertexInputRate::eVertex,
        };
        return {desc};
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
        auto uv_attr = vk::VertexInputAttributeDescription{
            3,
            0,
            vk::Format::eR32G32Sfloat,
            offsetof(Vertex, uv),
        };

        return {
            pos_attr,
            normal_attr,
            color_attr,
            uv_attr,
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
    ~Mesh();

    static Mesh quad(glm::vec3 color = {1.0f, 1.0f, 1.0f}) {
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

    static Mesh cube(float size = 1.0f, glm::vec3 color = {1.0f, 1.0f, 1.0f}) {
        Mesh mesh{};
        const auto s = size / 2.0f;

        std::vector<glm::vec3> normals{
            {-1.0f, 0.0f, 0.0f},
            {0.0f, -1.0f, 0.0f},
            {0.0f, 0.0f, -1.0f},
            {1.0f, 0.0f, 0.0f},
            {0.0f, 1.0f, 0.0f},
            {0.0f, 0.0f, 1.0f},
        };

        mesh._vertices = {
            // -X side
            {{-s, -s, -s}, normals[0], color, {0.0f, 0.625f}},
            {{-s, -s, s}, normals[0], color, {0.25f, 0.625f}},
            {{-s, s, s}, normals[0], color, {0.25f, 0.375f}},
            {{-s, s, s}, normals[0], color, {0.25f, 0.375f}},
            {{-s, s, -s}, normals[0], color, {0.0f, 0.375f}},
            {{-s, -s, -s}, normals[0], color, {0.0f, 0.625f}},

            // -Z side
            {{-s, -s, -s}, normals[2], color, {1.0f, 0.625f}},
            {{s, s, -s}, normals[2], color, {0.75f, 0.375f}},
            {{s, -s, -s}, normals[2], color, {0.75f, 0.625f}},
            {{-s, -s, -s}, normals[2], color, {1.0f, 0.625f}},
            {{-s, s, -s}, normals[2], color, {1.0f, 0.375f}},
            {{s, s, -s}, normals[2], color, {0.75f, 0.375f}},

            // -Y side
            {{-s, -s, -s}, normals[1], color, {0.25f, 0.875f}},
            {{s, -s, -s}, normals[1], color, {0.5f, 0.875f}},
            {{s, -s, s}, normals[1], color, {0.5f, 0.625f}},
            {{-s, -s, -s}, normals[1], color, {0.25f, 0.875f}},
            {{s, -s, s}, normals[1], color, {0.5f, 0.625f}},
            {{-s, -s, s}, normals[1], color, {0.25f, 0.625f}},

            // +Y side
            {{-s, s, -s}, normals[4], color, {0.25f, 0.125f}},
            {{-s, s, s}, normals[4], color, {0.25f, 0.375f}},
            {{s, s, s}, normals[4], color, {0.5f, 0.375f}},
            {{-s, s, -s}, normals[4], color, {0.25f, 0.125f}},
            {{s, s, s}, normals[4], color, {0.5f, 0.375f}},
            {{s, s, -s}, normals[4], color, {0.5f, 0.125f}},

            // +X side
            {{s, s, -s}, normals[3], color, {0.75f, 0.375f}},
            {{s, s, s}, normals[3], color, {0.5f, 0.375f}},
            {{s, -s, s}, normals[3], color, {0.5f, 0.625f}},
            {{s, -s, s}, normals[3], color, {0.5f, 0.625f}},
            {{s, -s, -s}, normals[3], color, {0.75f, 0.625f}},
            {{s, s, -s}, normals[3], color, {0.75f, 0.375f}},

            // +Z side
            {{-s, s, s}, normals[5], color, {0.25f, 0.375f}},
            {{-s, -s, s}, normals[5], color, {0.25f, 0.625f}},
            {{s, s, s}, normals[5], color, {0.5f, 0.375f}},
            {{-s, -s, s}, normals[5], color, {0.25f, 0.625f}},
            {{s, -s, s}, normals[5], color, {0.5f, 0.625f}},
            {{s, s, s}, normals[5], color, {0.5f, 0.375f}},
        };
        return mesh;
    }

    // implementation adapted from http://www.songho.ca/opengl/gl_sphere.html
    static Mesh sphere(
        float radius,
        u32 sectors,
        u32 stacks,
        glm::vec3 color = {1.0f, 1.0f, 1.0f}
    ) {
        Mesh mesh{};

        float d_sector = glm::two_pi<float>() / static_cast<float>(sectors);
        float d_step = glm::pi<float>() / static_cast<float>(stacks);

        for (u32 i = 0; i <= stacks; ++i) {
            auto theta = glm::half_pi<float>() - static_cast<float>(i) * d_step;
            auto xz = radius * glm::cos(theta);
            auto y = radius * glm::sin(theta);

            for (u32 j = 0; j <= sectors; ++j) {
                auto sector_angle = static_cast<float>(j) * d_sector;

                auto x = xz * glm::cos(sector_angle);
                auto z = xz * glm::sin(sector_angle);
                auto u = static_cast<float>(j) / static_cast<float>(sectors);
                auto v = static_cast<float>(i) / static_cast<float>(stacks);

                auto pos = glm::vec3{x, y, z};
                auto normal = glm::normalize(pos);
                auto uv = glm::vec2{1.0f - u, v};
                mesh._vertices.emplace_back(pos, normal, color, uv);
            }
        }

        for (u32 i = 0; i < stacks; ++i) {
            auto k1 = i * (sectors + 1);
            auto k2 = k1 + sectors + 1;

            for (u32 j = 0; j < sectors; ++j, ++k1, ++k2) {
                if (i != 0) {
                    mesh._indices.push_back(k1 + 1);
                    mesh._indices.push_back(k2);
                    mesh._indices.push_back(k1);
                }

                if (i != (stacks - 1)) {
                    mesh._indices.push_back(k2 + 1);
                    mesh._indices.push_back(k2);
                    mesh._indices.push_back(k1 + 1);
                }
            }
        }

        return mesh;
    }

    // implementation adapted from http://www.songho.ca/opengl/gl_cylinder.html
    static Mesh cylinder(
        float radius,
        float height,
        u32 sectors,
        glm::vec3 color = {1.0f, 1.0f, 1.0f}
    ) {
        Mesh mesh{};

        float h = height / 2.0f;

        // pre-compute sin/cos theta for reuse
        auto d_theta = glm::two_pi<float>() / static_cast<float>(sectors);
        std::vector<float> d_sin{};
        std::vector<float> d_cos{};
        for (u32 i = 0; i <= sectors; i++) {
            auto theta = d_theta * static_cast<float>(i);
            d_sin.push_back(radius * glm::sin(theta));
            d_cos.push_back(radius * glm::cos(theta));
        }

        // generate top circle vertices
        mesh._vertices.push_back(Vertex{
            {0.0f, h, 0.0f}, {0.0f, 1.0f, 0.0f}, color, {0.25f, 0.75f}});
        for (u32 i = 0; i <= sectors; i++) {
            float x = d_cos[i];
            float z = d_sin[i];
            // map uv to bottom left quadrant of texture
            float u = (x / radius + 1.0f) * 0.25f;
            float v = 0.5f + (z / radius + 1.0f) * 0.25f;

            mesh._vertices.push_back(
                {{x, h, z}, {0.0f, 1.0f, 0.0f}, color, {u, v}}
            );
        }

        // generate top circle indices
        for (u32 i = 1; i <= sectors; i++) {
            u32 j = (i + 1) % (sectors + 1);
            if (j == 0) {
                j++;
            }
            mesh._indices.push_back(i);
            mesh._indices.push_back(0);
            mesh._indices.push_back(j);
        }

        // generate bottom circle vertices
        u32 bottom_offset = static_cast<u32>(mesh._vertices.size());
        mesh._vertices.push_back(Vertex{
            {0.0f, -h, 0.0f}, {0.0f, -1.0f, 0.0f}, color, {0.25f, 0.75f}});
        for (u32 i = 0; i <= sectors; i++) {
            auto x = d_cos[i];
            auto z = d_sin[i];
            // map uv to bottom left quadrant of texture
            float u = (x / radius + 1.0f) * 0.25f;
            float v = 1.0f - (z / radius + 1.0f) * 0.25f;

            mesh._vertices.push_back(
                {{x, -h, z}, {0.0f, -1.0f, 0.0f}, color, {u, v}}
            );
        }

        // generate wall vertices (same as top/bottom, different normals)
        u32 wall_offset = static_cast<u32>(mesh._vertices.size());
        for (u32 i = 0; i <= sectors; i++) {
            auto x = d_cos[i];
            auto z = d_sin[i];
            glm::vec3 top{x, h, z};
            glm::vec3 bot{x, -h, z};
            glm::vec3 normal = glm::normalize(glm::vec3{x, 0.0f, z});
            float u =
                1.0f - static_cast<float>(i) / static_cast<float>(sectors);

            mesh._vertices.push_back(Vertex{bot, normal, color, {u, 0.5f}});
            mesh._vertices.push_back(Vertex{top, normal, color, {u, 0.0f}});
        }

        // generate bottom circle indices
        for (u32 i = 1; i <= sectors; i++) {
            u32 j = (i + 1) % (sectors + 1);
            if (j == 0) {
                j++;
            }
            // wind bottom triangles backwards to avoid culling
            mesh._indices.push_back(bottom_offset + j);
            mesh._indices.push_back(bottom_offset);
            mesh._indices.push_back(bottom_offset + i);
        }

        // generate cylinder wall indices
        auto end = sectors * 2;
        for (u32 i = 0; i <= end; i += 2) {
            u32 j = (i + 2) % (end + 2);

            // top and bottom vertices are interleaved
            auto k1 = wall_offset + i;
            auto k2 = wall_offset + i + 1;
            auto k3 = wall_offset + j + 1;
            auto k4 = wall_offset + j;
            mesh._indices.push_back(k1);
            mesh._indices.push_back(k2);
            mesh._indices.push_back(k3);
            mesh._indices.push_back(k1);
            mesh._indices.push_back(k3);
            mesh._indices.push_back(k4);
        }

        return mesh;
    }

    // derived with reference: https://electronut.in/torus
    static Mesh torus(
        float radius_ring,
        float radius_inner,
        u32 sectors,
        u32 segments,
        glm::vec3 color = {1.0f, 1.0f, 1.0f}
    ) {
        Mesh mesh{};

        auto d_theta = glm::two_pi<float>() / static_cast<float>(segments);
        auto d_phi = glm::two_pi<float>() / static_cast<float>(sectors);

        // generate circles along ring
        for (u32 i = 0; i <= segments; i++) {
            auto theta = static_cast<float>(i) * d_theta;
            auto u = 1.0f - (theta / glm::two_pi<float>());
            for (u32 j = 0; j <= sectors; j++) {
                auto phi = static_cast<float>(j) * d_phi;

                auto x = (radius_ring + radius_inner * glm::cos(phi)) *
                         glm::cos(theta);
                auto y = radius_inner * glm::sin(phi);
                auto z = (radius_ring + radius_inner * glm::cos(phi)) *
                         glm::sin(theta);
                auto v = 1.0f -
                         (static_cast<float>(j) / static_cast<float>(sectors));

                glm::vec3 vec{x, y, z};
                mesh._vertices.push_back(
                    {vec, glm::normalize(vec), color, {u, v}}
                );
            }
        }

        // generate indices along each segment
        for (u32 i = 0; i < segments; i++) {
            auto col = i * (sectors + 1);
            auto next_col = (i + 1) * (sectors + 1);
            for (u32 j = 0; j <= sectors; j++) {
                auto next_j = (j + 1) % (sectors + 1);
                auto k1 = col + j;
                auto k2 = col + next_j;
                auto k3 = next_col + next_j;
                auto k4 = next_col + j;

                mesh._indices.push_back(k1);
                mesh._indices.push_back(k2);
                mesh._indices.push_back(k3);
                mesh._indices.push_back(k1);
                mesh._indices.push_back(k3);
                mesh._indices.push_back(k4);
            }
        }

        return mesh;
    }

    [[nodiscard]]
    glm::mat4 transform() const;
    void upload(const vk::Queue& queue, UploadContext& ctx);
    void bind(const vk::UniqueCommandBuffer& cmd) const;
    void bind(const vk::CommandBuffer& cmd) const;
    void draw(const vk::UniqueCommandBuffer& cmd) const;
    void draw(const vk::CommandBuffer& cmd) const;
    void destroy();

    friend class Model;

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

    std::vector<Vertex> _vertices{};
    std::vector<u32> _indices{};

    AllocatedBuffer _vertex_buffer{};
    AllocatedBuffer _index_buffer{};
};

}  // namespace hvk
