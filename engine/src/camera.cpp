#include "camera.hpp"

namespace hc {

glm::mat4 Camera::view() const {
    return glm::lookAt(_pos, _pos + _front, _up);
}

glm::mat4 Camera::projection() const {
    return glm::perspective(glm::radians(_fov), _aspect, _near_z, _far_z);
}

glm::vec3 Camera::translation() const {
    return _pos;
}

void Camera::set_aspect(float aspect) {
    _aspect = aspect;
}

void Camera::set_translation(glm::vec3 pos) {
    _pos = pos;
}

void Camera::set_sprint(bool on) {
    _sprint = on;
}

void Camera::rotate(double dx, double dy) {
    auto up_sign = (_up[1] > 0.0f ? -1.0f : 1.0f);
    _yaw += up_sign * static_cast<float>(dx) * _rotation_speed;
    if (_yaw < -180.f) {
        _yaw += 360.f;
    }
    if (_yaw > 180.f) {
        _yaw -= 360.f;
    }
    _pitch += static_cast<float>(dy) * _rotation_speed;
    _pitch = std::clamp(_pitch, -85.f, 85.f);

    auto rotation =
        glm::quat{glm::vec3{glm::radians(_pitch), glm::radians(_yaw), 0.f}};
    _front = glm::normalize(rotation * glm::vec3{0.f, 0.f, 1.f});
    spdlog::debug(
        "[camera] Rotate: yaw={}, pitch={}, front=[{}, {}, {}]",
        _yaw,
        _pitch,
        _front.x,
        _front.y,
        _front.z
    );
}

void Camera::move(CameraDirection direction, double dt) {
    auto amount = _speed * static_cast<float>(dt);
    if (_sprint) {
        amount *= 4.0f;
    }

    switch (direction) {
        case CameraDirection::Forward:
            _pos += amount * _front;
            break;
        case CameraDirection::Backward:
            _pos -= amount * _front;
            break;
        case CameraDirection::Left:
            _pos -= glm::normalize(glm::cross(_front, _up)) * amount;
            break;
        case CameraDirection::Right:
            _pos += glm::normalize(glm::cross(_front, _up)) * amount;
            break;
        case CameraDirection::Up: {
            auto right = glm::cross(_front, _up);
            _pos -= glm::normalize(glm::cross(_front, right)) * amount;
            break;
        }
        case CameraDirection::Down: {
            auto right = glm::cross(_front, _up);
            _pos += glm::normalize(glm::cross(_front, right)) * amount;
            break;
        }
        default:
            break;
    }
    spdlog::debug("[camera] Move: pos=[{}, {}, {}]", _pos.x, _pos.y, _pos.z);
}

void Camera::zoom(ZoomDirection direction, double dt) {
    auto delta = glm::degrees(static_cast<float>(dt) * _zoom_speed);
    delta *= direction == ZoomDirection::In ? -1.0f : 1.0f;
    _fov = std::clamp(_fov + delta, 10.0f, 100.0f);
    spdlog::debug("[camera] Zoom: fov={}", _fov);
}

}  // namespace hc
