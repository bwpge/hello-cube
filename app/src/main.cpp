#include "hello_cube.hpp"

i32 main() {
    hc::Engine engine{"Hello Triangle", 1600, 900};
    engine.init();
    engine.run();
    engine.cleanup();
}
