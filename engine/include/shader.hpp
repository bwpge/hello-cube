#pragma once

#include <fstream>
#include <filesystem>
#include <map>

#include <vulkan/vulkan.hpp>

#include "core.hpp"

namespace hc {

class Shader {
public:
    static Shader load_spv(const std::filesystem::path& path) {
        std::ifstream file{path, std::ios::ate | std::ios::binary};
        Shader result{};

        if (!file.is_open()) {
            PANIC(spdlog::fmt_lib::format(
                "failed to open shader file '{}'", path.string()
            ));
        }

        auto size = file.tellg();
        if (size <= 0) {
            PANIC("shader contains no byte code");
        }

        result._buf.resize(size);
        file.seekg(0);
        file.read(result._buf.data(), size);

        return result;
    }

    [[nodiscard]]
    vk::UniqueShaderModule shader_module(const vk::Device& device);

private:
    std::vector<char> _buf{};
};

struct ShaderMap {
    std::unordered_map<std::string_view, Shader> vertex{};
    std::unordered_map<std::string_view, Shader> fragment{};
};

}  // namespace hc
