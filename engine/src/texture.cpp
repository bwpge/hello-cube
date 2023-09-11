#define STB_IMAGE_IMPLEMENTATION
#include "hvk/texture.hpp"

namespace hvk {

Texture::Texture(Texture&& other) noexcept {
    std::swap(_image, other._image);
}

Texture& Texture::operator=(Texture&& rhs) noexcept {
    if (this == &rhs) {
        return *this;
    }

    destroy();
    std::swap(_image, rhs._image);

    return *this;
}

Texture::~Texture() {
    destroy();
}

void Texture::destroy() {
    VulkanContext::allocator().destroy(_image);
}

}  // namespace hvk
