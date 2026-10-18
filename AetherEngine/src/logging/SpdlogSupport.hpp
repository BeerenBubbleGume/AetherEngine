#ifndef AETHERENGINE_LOGGING_SPDLOGSUPPORT_HPP
#define AETHERENGINE_LOGGING_SPDLOGSUPPORT_HPP

#include <spdlog/common.h>

#include "logging/LogLevel.hpp"

// Backend types stay private to the engine implementation.
namespace AetherEngine::logging::detail {
    constexpr auto toSpdlogLevel(LogLevel level) noexcept -> spdlog::level::level_enum {
        switch (level) {
            case LogLevel::Trace: return spdlog::level::trace;
            case LogLevel::Debug: return spdlog::level::debug;
            case LogLevel::Info: return spdlog::level::info;
            case LogLevel::Warning: return spdlog::level::warn;
            case LogLevel::Error: return spdlog::level::err;
            case LogLevel::Fatal: return spdlog::level::critical;
        }
        return spdlog::level::off;
    }

    constexpr auto fromSpdlogLevel(spdlog::level::level_enum level) noexcept -> LogLevel {
        switch (level) {
            case spdlog::level::trace: return LogLevel::Trace;
            case spdlog::level::debug: return LogLevel::Debug;
            case spdlog::level::info: return LogLevel::Info;
            case spdlog::level::warn: return LogLevel::Warning;
            case spdlog::level::err: return LogLevel::Error;
            case spdlog::level::critical: return LogLevel::Fatal;
            default: return LogLevel::Info;
        }
    }

    inline auto toSpdlogView(std::string_view text) noexcept -> spdlog::string_view_t {
        return {text.data(), text.size()};
    }
}

#endif
