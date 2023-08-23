#pragma once

#include <spdlog/spdlog.h>

#include "types.hpp"

#define PANIC(msg)                                                      \
    do {                                                                \
        spdlog::critical("PANIC: {} ({}:{})", msg, __FILE__, __LINE__); \
        throw std::runtime_error(msg);                                  \
    } while (0)
