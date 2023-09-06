#include "timer.hpp"

namespace hvk {

Timer::Timer() {
    reset();
}

double Timer::tick() {
    auto dt = elapsed_secs();
    _last = Clock::now();
    return dt;
}

void Timer::reset() {
    _start = Clock::now();
    _last = _start;
}

double Timer::elapsed_secs() {
    return elapsed_ms() / 1000.0;
}

double Timer::elapsed_ms() {
    // https://en.cppreference.com/w/cpp/chrono/duration/duration_cast
    auto millis =
        std::chrono::duration<double, std::milli>(Clock::now() - _last);
    return millis.count();
}

double Timer::total_secs() {
    return total_ms() / 1000.0;
}

double Timer::total_ms() {
    auto millis =
        std::chrono::duration<double, std::milli>(Clock::now() - _start);
    return millis.count();
}

}  // namespace hvk
