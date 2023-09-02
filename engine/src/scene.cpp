#include "scene.hpp"

namespace hc {

Mesh& Scene::add_mesh(Mesh&& mesh) {
    _meshes.push_back(std::move(mesh));
    return _meshes.back();
}

void Scene::destroy() {
    for (auto& mesh : _meshes) {
        mesh.destroy();
    }
}

}  // namespace hc
