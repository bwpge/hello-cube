#include <memory>

#include "hvk/hello_vulkan.hpp"

int main() {
    auto engine =
        std::make_unique<hvk::Engine>("Hello Vulkan", 1920, 1080, hvk::BufferingMode::Double);

    engine->init();
    engine->run();
    engine->cleanup();
}
