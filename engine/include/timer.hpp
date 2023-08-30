#pragma once

#include <chrono>

namespace hc {

using Clock = std::chrono::high_resolution_clock;
using Instant = std::chrono::time_point<Clock>;

class Timer {
public:
    Timer();

    double tick();
    void reset();
    double elapsed_secs();
    double elapsed_ms();

private:
    Instant _start{};
};

}  // namespace hc
