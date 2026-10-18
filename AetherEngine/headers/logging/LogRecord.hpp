#ifndef AETHERENGINE_LOGGING_LOGRECORD_HPP
#define AETHERENGINE_LOGGING_LOGRECORD_HPP

#include <chrono>
#include <cstdint>
#include <string>
#include <thread>

#include "LogLevel.hpp"

namespace AetherEngine::logging {
    struct LogRecord {
        std::uint64_t sequence{};
        std::chrono::system_clock::time_point timestamp{};
        std::thread::id threadId{};
        LogLevel level{LogLevel::Info};
        std::string category;
        std::string message;
        bool truncated{};
    };
}

#endif
