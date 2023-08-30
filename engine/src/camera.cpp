#include "camera.hpp"

namespace hc {

glm::mat4 Camera::get_view() const {
    return glm::translate(glm::mat4(1.0f), _pos);
}

glm::mat4 Camera::get_projection() const {
    return glm::perspective(_fov, _aspect, _near_z, _far_z);
}

void Camera::set_aspect(float aspect) {
    _aspect = aspect;
}

void Camera::set_position(glm::vec3 pos) {
    _pos = pos;
}

}  // namespace hc
