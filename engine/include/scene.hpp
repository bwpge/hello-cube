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

private:
    std::vector<Mesh> _meshes{};
};

}  // namespace hc
