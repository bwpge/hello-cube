#include "scene.hpp"

namespace hc {

Mesh& Scene::add_mesh(Mesh&& mesh) {
    _meshes.push_back(std::move(mesh));
    return _meshes.back();
}

void Scene::draw(const vk::UniqueCommandBuffer& cmd) const {
    draw(cmd.get());
}

void Scene::draw(const vk::CommandBuffer& cmd) const {
    for (const auto& mesh : _meshes) {
        mesh.draw(cmd);
    }
}

void Scene::destroy() {
    for (auto& mesh : _meshes) {
        mesh.destroy();
    }
}

}  // namespace hc
