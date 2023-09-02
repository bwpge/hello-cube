#pragma once

#include "core.hpp"
#include "mesh.hpp"

namespace hc {

class Scene {
public:
    Mesh& add_mesh(Mesh&& mesh);
    void draw(const vk::UniqueCommandBuffer& cmd) const;
    void draw(const vk::CommandBuffer& cmd) const;
    void destroy();

    [[nodiscard]]
    const std::vector<Mesh>& meshes() const {
        return _meshes;
    }

    [[nodiscard]]
    std::vector<Mesh>& meshes() {
        return _meshes;
    }

    [[nodiscard]]
    glm::vec3 light_pos() const {
        return _light_pos;
    }

    void set_light_pos(const glm::vec3& light_pos) {
        _light_pos = light_pos;
    }

private:
    std::vector<Mesh> _meshes{};
    glm::vec3 _light_pos{};
};

}  // namespace hc
