#pragma once

#include "core.hpp"
#include "mesh.hpp"

namespace hvk {

class Scene {
public:
    template <typename T>
    void add_mesh(T&& mesh) {
        _meshes.push_back(std::forward<T>(mesh));
    }

    [[nodiscard]]
    const std::vector<Mesh>& meshes() const {
        return _meshes;
    }

    [[nodiscard]]
    std::vector<Mesh>& meshes() {
        return _meshes;
    }

    [[nodiscard]]
    inline glm::vec3 light_pos() const {
        return _light_pos;
    }

    inline void set_light_pos(const glm::vec3& light_pos) {
        _light_pos = light_pos;
    }

private:
    std::vector<Mesh> _meshes{};
    glm::vec3 _light_pos{};
};

}  // namespace hvk
