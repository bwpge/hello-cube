#pragma once

#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>

namespace hc {

class Camera {
public:
    Camera(float fov, float aspect, float near_z, float far_z)
        : _fov{fov},
          _aspect{aspect},
          _near_z{near_z},
          _far_z{far_z} {}

    Camera() = default;
    Camera(const Camera&) = default;
    Camera(Camera&&) noexcept = default;
    Camera& operator=(const Camera&) = default;
    Camera& operator=(Camera&& rhs) noexcept = default;
    ~Camera() = default;

    [[nodiscard]]
    glm::mat4 get_view() const;
    [[nodiscard]]
    glm::mat4 get_projection() const;
    void set_aspect(float aspect);
    void set_position(glm::vec3 pos);

private:
    glm::vec3 _pos{0.f, 0.f, -2.f};
    float _fov{1.22173f};  // 70 degrees
    float _aspect{1.78f};  // 16:9
    float _near_z{0.1f};
    float _far_z{200.f};
};

}  // namespace hc
