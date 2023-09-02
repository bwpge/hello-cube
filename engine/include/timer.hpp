#pragma once

#include <chrono>

namespace hc {

class Timer {
public:
    using Clock = std::chrono::high_resolution_clock;
    using Instant = std::chrono::time_point<Clock>;

    Timer();

    double tick();
    void reset();
    double elapsed_secs();
    double elapsed_ms();
    double total_secs();
    double total_ms();

private:
    Instant _start{};
    Instant _last{};
};

}  // namespace hc
