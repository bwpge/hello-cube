#include "hello_cube.hpp"

i32 main() {
    hc::Engine engine{"Hello Triangle", 1600, 900};

    try {
        engine.init();
        engine.run();
        engine.cleanup();
    } catch (const std::exception& e) {
        spdlog::critical("UNHANDLED EXCEPTION: {}", e.what());
    } catch (...) {
        spdlog::critical("UNHANDLED EXCEPTION");
    }
}
