#pragma once

#include <filesystem>
#include <fstream>

#include <vulkan/vulkan.hpp>

#include "hvk/core.hpp"

namespace hvk {

class Shader {
public:
    static Shader load_spv(const std::filesystem::path& path) {
        spdlog::trace("Loading shader: '{}'", path.string());
        std::ifstream file{path, std::ios::ate | std::ios::binary};
        Shader result{};

        if (!file.is_open()) {
            panic(fmt::format("failed to open shader file '{}'", path.string()));
        }

        auto size = file.tellg();
        if (size <= 0) {
            panic("shader does not contain any data");
        }

        result._buf.resize(size);
        file.seekg(0);
        file.read(result._buf.data(), size);

        return result;
    }

    [[nodiscard]]
    vk::UniqueShaderModule module() const;

private:
    std::vector<char> _buf{};
};

enum class ShaderType {
    Vertex,
    Fragment,
    Geometry,
    Compute
};

}  // namespace hvk

template<>
struct fmt::formatter<hvk::ShaderType> {
    template<typename ParseContext>
    constexpr auto parse(ParseContext& ctx) {
        return ctx.begin();
    }

    template<typename FormatContext>
    auto format(hvk::ShaderType type, FormatContext& ctx) {
        std::string result{};
        switch (type) {
            case hvk::ShaderType::Vertex:
                result = "Vertex";
                break;
            case hvk::ShaderType::Fragment:
                result = "Fragment";
                break;
            case hvk::ShaderType::Geometry:
                result = "Geometry";
                break;
            case hvk::ShaderType::Compute:
                result = "Compute";
                break;
        }

        return fmt::format_to(ctx.out(), "{0}", result);
    }
};
