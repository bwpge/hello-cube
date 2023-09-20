#pragma once

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/transform.hpp>

#include "hvk/core.hpp"
#include "hvk/material.hpp"
#include "hvk/mesh.hpp"
#include "hvk/resource_manager.hpp"

namespace hvk {

struct Transform {
    glm::vec3 translation{0.0f};
    glm::vec3 rotation{0.0f};
    glm::vec3 scale{1.0f};
};

struct Node {
    Material* material{};
    usize mesh_idx{};
};

class Model {
public:
    static Model quad(Material* material) {
        Model model{};
        model._meshes.push_back(Mesh::quad());
        model._materials.push_back(material);
        model._nodes.push_back({material, 0});
        return model;
    }

    static Model cube(Material* material, float size = 1.0f) {
        Model model{};
        model._meshes.push_back(Mesh::cube(size));
        model._materials.push_back(material);
        model._nodes.push_back({material, 0});
        return model;
    }

    static Model sphere(Material* material, float radius, u32 sectors, u32 stacks) {
        Model model{};
        model._meshes.push_back(Mesh::sphere(radius, sectors, stacks));
        model._materials.push_back(material);
        model._nodes.push_back({material, 0});
        return model;
    }

    static Model cylinder(Material* material, float radius, float height, u32 sectors) {
        Model model{};
        model._meshes.push_back(Mesh::cylinder(radius, height, sectors));
        model._materials.push_back(material);
        model._nodes.push_back({material, 0});
        return model;
    }

    static Model
    torus(Material* material, float radius_ring, float radius_inner, u32 sectors, u32 segments) {
        Model model{};
        model._meshes.push_back(Mesh::torus(radius_ring, radius_inner, sectors, segments));
        model._materials.push_back(material);
        model._nodes.push_back({material, 0});
        return model;
    }

    static Model load_obj(const std::filesystem::path& path, UploadContext& ctx) {
        spdlog::trace("Loading mesh: {}", path.string());
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;

        std::string warn;
        std::string err;
        auto mtl_base_dir = path.parent_path();

        tinyobj::LoadObj(
            &attrib,
            &shapes,
            &materials,
            &warn,
            &err,
            path.string().c_str(),
            mtl_base_dir.string().c_str()
        );
        if (!warn.empty()) {
            spdlog::warn("[tiny_obj_loader] {}", warn);
        }
        if (!err.empty()) {
            panic(fmt::format("[tiny_obj_loader] {}", err));
        }

        Model model{};
        model._materials = load_obj_materials(mtl_base_dir, materials, ctx);

        const usize vert_count = 3;
        Mesh mesh{};
        i32 mat_id{};
        i32 last_mat_id{};

        for (auto& shape : shapes) {
            usize offset = 0;
            for (usize i = 0; i < shape.mesh.num_face_vertices.size(); i++) {
                // check for new materials for each face,
                // create a new mesh if material changed
                if (i % 3 == 0) {
                    mat_id = shape.mesh.material_ids[i / 3];
                    if (mat_id != last_mat_id) {
                        if (!mesh._vertices.empty()) {
                            model._nodes.push_back(Node{
                                model._materials.at(static_cast<usize>(last_mat_id)),
                                model._meshes.size(),
                            });
                            model._meshes.push_back(std::move(mesh));
                            mesh = {};
                        }
                        last_mat_id = mat_id;
                    }
                }

                // hardcode loading triangles
                for (usize j = 0; j < vert_count; j++) {
                    tinyobj::index_t idx = shape.mesh.indices[offset + j];

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

                    // important to flip y coordinate for vulkan space
                    vertex.uv = {
                        attrib.texcoords[2 * idx.texcoord_index + 0],
                        1.0f - attrib.texcoords[2 * idx.texcoord_index + 1],
                    };

                    mesh._vertices.push_back(vertex);
                }
                offset += vert_count;
            }
        }
        if (!mesh._vertices.empty()) {
            model._nodes.push_back(Node{
                model._materials.at(static_cast<usize>(last_mat_id)),
                model._meshes.size(),
            });
            model._meshes.push_back(std::move(mesh));
        }

        return model;
    }

    template<typename T>
    void add_mesh(T&& mesh) {
        _meshes.push_back(std::forward<T>(mesh));
    }

    [[nodiscard]]
    glm::mat4 transform() const;
    [[nodiscard]]
    const std::vector<Node>& nodes() const;

    void translate(glm::vec3 translation);
    void set_translation(glm::vec3 position);
    void rotate(glm::vec3 rotation);
    void set_rotation(glm::vec3 rotation);
    void scale(float scale);
    void set_scale(float scale);

    void upload(const vk::Queue& queue, UploadContext& ctx);
    void draw(const vk::UniqueCommandBuffer& cmd) const;
    void draw(const vk::CommandBuffer& cmd) const;
    void draw_node(const Node& node, const vk::UniqueCommandBuffer& cmd) const;

private:
    static std::vector<Material*> load_obj_materials(
        const std::filesystem::path& base_dir,
        const std::vector<tinyobj::material_t>& materials,
        UploadContext& ctx
    ) {
        std::vector<Material*> result{};
        for (const auto& mat : materials) {
            glm::vec3 ambient_base = glm::vec3{mat.ambient[0], mat.ambient[1], mat.ambient[2]};
            auto ambient_tex = std::filesystem::path{mat.ambient_texname};
            result.push_back(
                ResourceManager::make_material(mat.name, base_dir, ambient_base, ambient_tex, ctx)
            );
        }

        return result;
    }

    Transform _transform{};
    std::vector<Mesh> _meshes{};
    std::vector<Material*> _materials{};
    std::vector<Node> _nodes{};
};

}  // namespace hvk
