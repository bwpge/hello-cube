#include "logger.hpp"

#include <spdlog/sinks/stdout_color_sinks.h>

namespace hvk {

void configure_logger() {
    auto sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
#ifdef _WIN32
    sink->set_color(spdlog::level::trace, FOREGROUND_INTENSITY);
    sink->set_color(spdlog::level::debug, FOREGROUND_GREEN | FOREGROUND_BLUE);
    sink->set_color(spdlog::level::info, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
    sink->set_color(spdlog::level::warn, FOREGROUND_RED | FOREGROUND_GREEN);
    sink->set_color(spdlog::level::err, FOREGROUND_RED);
    sink->set_color(
        spdlog::level::critical,
        FOREGROUND_WHITE | FOREGROUND_INTENSITY | BACKGROUND_RED
    );
#else
    sink->set_color(spdlog::level::trace, "\033[2;30m");
    sink->set_color(spdlog::level::debug, "\033[36m");
    sink->set_color(spdlog::level::info, "\033[0m");
    sink->set_color(spdlog::level::warn, "\033[33m");
    sink->set_color(spdlog::level::err, "\033[31m");
    sink->set_color(spdlog::level::critical, "\033[41m\033[37m");
#endif  // _WIN32

    auto logger = std::make_shared<spdlog::logger>(spdlog::logger{"", sink});
    logger->set_level(spdlog::level::trace);
    logger->set_pattern("%^[%T.%e] [%t] [%l] %v%$");
    spdlog::set_default_logger(logger);
}

}  // namespace hvk
