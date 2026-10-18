#ifndef AETHERENGINE_LOGGING_LOGGER_HPP
#define AETHERENGINE_LOGGING_LOGGER_HPP

#include <cstddef>
#include <memory>
#include <string_view>
#include <vector>

#include "ILogSink.hpp"

namespace AetherEngine::logging {
    // All public operations may run concurrently. Join logging threads before
    // destroying the logger. Owns a synchronous backend via RAII; no global
    // registration, worker thread, or unbounded pending queue is used.
    class Logger final {
    public:
        static constexpr std::size_t DefaultCapacity = 1024;
        static constexpr std::size_t MaxMessageBytes = 4096;
        static constexpr std::size_t MaxCategoryBytes = 128;

        explicit Logger(std::size_t capacity = DefaultCapacity);
        ~Logger();
        Logger(const Logger&) = delete;
        auto operator=(const Logger&) -> Logger& = delete;
        Logger(Logger&&) = delete;
        auto operator=(Logger&&) -> Logger& = delete;

        auto log(LogLevel level, std::string_view message, std::string_view category = {}) -> void;
        auto trace(std::string_view message, std::string_view category = {}) -> void;
        auto debug(std::string_view message, std::string_view category = {}) -> void;
        auto info(std::string_view message, std::string_view category = {}) -> void;
        auto warning(std::string_view message, std::string_view category = {}) -> void;
        auto error(std::string_view message, std::string_view category = {}) -> void;
        // Records a fatal diagnostic; it does not terminate the process.
        auto fatal(std::string_view message, std::string_view category = {}) -> void;

        auto setMinLevel(LogLevel level) -> void;
        [[nodiscard]] auto minLevel() const -> LogLevel;

        // The ring retains the newest records. Zero disables history, but sinks
        // still receive logs. Resizing preserves the newest records that fit.
        auto setCapacity(std::size_t capacity) -> void;
        [[nodiscard]] auto capacity() const -> std::size_t;
        [[nodiscard]] auto snapshot() const -> std::vector<LogRecord>;
        // Existing snapshots remain valid; sequence numbers are never reset.
        auto clear() -> void;

        // Shared ownership keeps sinks alive during in-flight writes. Adding a
        // null or already registered sink is a no-op. No history is replayed.
        auto addSink(std::shared_ptr<ILogSink> sink) -> void;
        // In-flight writes may still finish after removal returns.
        auto removeSink(std::shared_ptr<ILogSink> sink) -> void;

    private:
        class Impl;
        std::unique_ptr<Impl> m_impl;
    };
}

#endif
