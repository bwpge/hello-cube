#pragma once

#include <glm/glm.hpp>

#include "hvk/allocator.hpp"
#include "hvk/model.hpp"

namespace hvk {

struct SceneData {
    glm::vec4 light_color;
    glm::vec4 light_dir;
};

class Scene {
public:
    template<typename T>
    void add_model(T&& model) {
        _models.push_back(std::forward<T>(model));
    }

    [[nodiscard]]
    const std::vector<Model>& models() const;
    [[nodiscard]]
    std::vector<Model>& models();
    [[nodiscard]]
    glm::vec3 light_dir() const;
    void set_light_dir(const glm::vec3& direction);
    [[nodiscard]]
    glm::vec3 light_color() const;
    void set_light_color(const glm::vec3& color);
    [[nodiscard]]
    SceneData data() const;

private:
    std::vector<Model> _models{};
    glm::vec3 _dir{glm::normalize(glm::vec3{0.0f, 1.0f, 1.0f})};
    glm::vec3 _color{1.0f};
    AllocatedBuffer _buf{};
};

}  // namespace hvk
