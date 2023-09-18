#include "hvk/model.hpp"

namespace hvk {

glm::mat4 Model::transform() const {
    auto translate = glm::translate(glm::mat4(1.0f), _transform.translation);
    auto rotate = glm::toMat4(glm::quat(_transform.rotation));
    auto scale = glm::scale(glm::mat4(1.0f), _transform.scale);
    return translate * rotate * scale;
}

const std::vector<Node>& Model::nodes() const {
    return _nodes;
}

void Model::rotate(glm::vec3 rotation) {
    _transform.rotation += rotation;
}

void Model::set_rotation(glm::vec3 rotation) {
    _transform.rotation = rotation;
}

void Model::translate(glm::vec3 translation) {
    _transform.translation += translation;
}

void Model::set_translation(glm::vec3 position) {
    _transform.translation = position;
}

void Model::scale(float scale) {
    _transform.scale += glm::vec3{scale};
}

void Model::set_scale(float scale) {
    _transform.scale = glm::vec3{scale};
}

void Model::upload(const vk::Queue& queue, UploadContext& ctx) {
    for (auto& mesh : _meshes) {
        mesh.upload(queue, ctx);
    }
}

void Model::draw(const vk::UniqueCommandBuffer& cmd) const {
    draw(cmd.get());
}

void Model::draw(const vk::CommandBuffer& cmd) const {
    for (const auto& [_, mesh_idx] : _nodes) {
        const auto& mesh = _meshes.at(mesh_idx);
        mesh.bind(cmd);
        mesh.draw(cmd);
    }
}

void Model::draw_node(const Node& node, const vk::UniqueCommandBuffer& cmd)
    const {
    const auto& mesh = _meshes.at(node.mesh_idx);
    mesh.bind(cmd);
    mesh.draw(cmd);
}

}  // namespace hvk
