#include "hvk/scene.hpp"

namespace hvk {

const std::vector<Mesh>& Scene::meshes() const {
    return _meshes;
}

std::vector<Mesh>& Scene::meshes() {
    return _meshes;
}

glm::vec3 Scene::light_dir() const {
    return _dir;
}

void Scene::set_light_dir(const glm::vec3& direction) {
    _dir = direction;
}

glm::vec3 Scene::light_color() const {
    return _color;
}

void Scene::set_light_color(const glm::vec3& color) {
    _color = color;
}

SceneData Scene::data() const {
    return {
        glm::vec4{_color, 1.0f},
        glm::vec4{_dir, 0.0f},
    };
}

}  // namespace hvk
