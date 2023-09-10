#include <memory>

#include "hvk/hello_vulkan.hpp"

int main() {
    auto engine = std::make_unique<hvk::Engine>(
        "Hello Vulkan", 1920, 1080, hvk::BufferingMode::Double
    );

    try {
        engine->init();
        engine->run();
        engine->cleanup();
    } catch (const std::exception& e) {
        spdlog::critical("UNHANDLED EXCEPTION: {}", e.what());
    } catch (...) {
        spdlog::critical("UNHANDLED EXCEPTION");
    }
}
