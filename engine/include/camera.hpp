#pragma once

#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/quaternion.hpp>

#include "core.hpp"

namespace hc {

enum class CameraDirection {
    Forward,
    Backward,
    Left,
    Right,
    Up,
    Down,
};

enum class ZoomDirection {
    In,
    Out,
};

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
    [[nodiscard]]
    glm::vec3 get_position() const;

    void set_aspect(float aspect);
    void set_position(glm::vec3 pos);
    void set_sprint(bool on);

    inline void reset() {
        _pos = _start;
        _pitch = 0.f;
        _yaw = -180.f;
        _fov = 45.f;
        _front = {0.f, 0.f, -1.f};
    }

    void rotate(double dx, double dy);
    void move(CameraDirection direction, double dt);
    void zoom(ZoomDirection direction, double dt);

private:
    glm::vec3 _pos{0.f, 0.f, 5.f};
    glm::vec3 _start{_pos};
    glm::vec3 _front{0.f, 0.f, -1.f};
    glm::vec3 _up{0.f, 1.f, 0.f};
    float _pitch{};
    float _yaw{-180.f};  // start at -180 to look at origin
    float _fov{45.f};
    float _aspect{1.78f};  // 16:9
    float _near_z{0.1f};
    float _far_z{200.f};

    bool _sprint{};
    float _speed{5.f};
    float _rotation_speed{0.05f};
    float _zoom_speed{5.f};
};

}  // namespace hc
