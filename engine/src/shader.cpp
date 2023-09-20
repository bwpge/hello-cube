#include "hvk/shader.hpp"
#include "hvk/vk_context.hpp"

namespace hvk {

vk::UniqueShaderModule Shader::module() const {
    auto info = vk::ShaderModuleCreateInfo{};
    info.setPCode(reinterpret_cast<const u32*>(_buf.data())).setCodeSize(_buf.size());
    return VulkanContext::device().createShaderModuleUnique(info);
}

}  // namespace hvk
