#include "scene.hpp"

namespace hc {

void Scene::destroy() {
    for (auto& mesh : _meshes) {
        mesh.destroy();
    }
}

}  // namespace hc
