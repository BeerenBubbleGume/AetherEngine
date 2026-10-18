#ifndef AETHERENGINE_LOGGING_LOGLEVEL_HPP
#define AETHERENGINE_LOGGING_LOGLEVEL_HPP

#include <cstdint>
#include <string_view>

namespace AetherEngine::logging {
    enum class LogLevel : std::uint8_t {
        Trace,
        Debug,
        Info,
        Warning,
        Error,
        Fatal
    };

    [[nodiscard]] constexpr auto toString(LogLevel level) noexcept -> std::string_view {
        switch (level) {
            case LogLevel::Trace: return "Trace";
            case LogLevel::Debug: return "Debug";
            case LogLevel::Info: return "Info";
            case LogLevel::Warning: return "Warning";
            case LogLevel::Error: return "Error";
            case LogLevel::Fatal: return "Fatal";
        }
        return "Unknown";
    }
}

#endif
