#include "timer.hpp"

namespace hc {

Timer::Timer() {
    reset();
}

double Timer::tick() {
    auto dt = elapsed_secs();
    _start = Clock::now();
    return dt;
}

void Timer::reset() {
    _start = Clock::now();
}

double Timer::elapsed_secs() {
    return elapsed_ms() / 1000.0;
}

double Timer::elapsed_ms() {
    // https://en.cppreference.com/w/cpp/chrono/duration/duration_cast
    auto millis =
        std::chrono::duration<double, std::milli>(Clock::now() - _start);
    return millis.count();
}

}  // namespace hc
