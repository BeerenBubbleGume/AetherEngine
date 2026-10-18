#include "logging/Logger.hpp"

#include <algorithm>
#include <atomic>
#include <exception>
#include <mutex>
#include <utility>

#include <spdlog/logger.h>
#include <spdlog/sinks/sink.h>

#include "logging/SpdlogSupport.hpp"

namespace AetherEngine::logging {
    namespace {
        auto boundedText(spdlog::string_view_t text, std::size_t limit) -> std::string {
            if (text.size() > limit) {
                // Preserve code point boundaries in valid UTF-8.
                while (limit > 0 && (static_cast<unsigned char>(text[limit]) & 0xc0) == 0x80) {
                    --limit;
                }
            } else {
                limit = text.size();
            }
            return limit == 0 ? std::string{} : std::string{text.data(), limit};
        }

        // Keeps the engine-specific structured history and ILogSink contract.
        // It deliberately does not use base_sink<mutex>: callbacks may reenter
        // the logger, so no history/subscription lock may be held across them.
        class RecordSink final : public spdlog::sinks::sink {
        public:
            explicit RecordSink(std::size_t capacity) : m_records(capacity) {}

            auto log(const spdlog::details::log_msg& message) -> void override {
                auto record = std::make_shared<LogRecord>(LogRecord{
                    .timestamp = message.time,
                    // The backend is synchronous, preserving std::thread::id in
                    // the public API rather than exposing spdlog's native ID.
                    .threadId = std::this_thread::get_id(),
                    .level = detail::fromSpdlogLevel(message.level),
                    .category = boundedText(message.logger_name, Logger::MaxCategoryBytes),
                    .message = boundedText(message.payload, Logger::MaxMessageBytes),
                    .truncated = message.payload.size() > Logger::MaxMessageBytes ||
                        message.logger_name.size() > Logger::MaxCategoryBytes
                });
                const auto sinks = m_sinks.load();
                {
                    const std::lock_guard lock{m_historyMutex};
                    record->sequence = m_nextSequence++;
                    if (!m_records.empty()) {
                        m_records[(m_start + m_size) % m_records.size()] = record;
                        if (m_size == m_records.size()) {
                            m_start = (m_start + 1) % m_records.size();
                        } else {
                            ++m_size;
                        }
                    }
                }
                for (const auto& sink : *sinks) {
                    try {
                        sink->write(*record);
                    } catch (...) {
                        // Preserve ILogSink's exception isolation, including
                        // non-std exceptions, without recursively logging.
                    }
                }
            }

            // This sink retains raw structured records, not formatted output.
            auto flush() -> void override {}
            auto set_pattern(const std::string&) -> void override {}
            auto set_formatter(std::unique_ptr<spdlog::formatter>) -> void override {}

            auto setCapacity(std::size_t capacity) -> void {
                std::vector<RecordPtr> records(capacity);
                {
                    const std::lock_guard lock{m_historyMutex};
                    const auto retained = std::min(m_size, capacity);
                    for (std::size_t i = 0; i < retained; ++i) {
                        records[i] = std::move(m_records[(m_start + m_size - retained + i) % m_records.size()]);
                    }
                    m_records.swap(records);
                    m_start = 0;
                    m_size = retained;
                }
            }

            auto capacity() const -> std::size_t {
                const std::lock_guard lock{m_historyMutex};
                return m_records.size();
            }

            auto snapshot() const -> std::vector<LogRecord> {
                std::vector<RecordPtr> retained;
                {
                    const std::lock_guard lock{m_historyMutex};
                    retained.reserve(m_size);
                    for (std::size_t i = 0; i < m_size; ++i) {
                        retained.push_back(m_records[(m_start + i) % m_records.size()]);
                    }
                }
                // Copy text after releasing the history lock; concurrent
                // overwrite/clear cannot invalidate these immutable records.
                std::vector<LogRecord> records;
                records.reserve(retained.size());
                for (const auto& record : retained) {
                    records.push_back(*record);
                }
                return records;
            }

            auto clear() -> void {
                const std::lock_guard lock{m_historyMutex};
                for (auto& record : m_records) {
                    record.reset();
                }
                m_start = 0;
                m_size = 0;
            }

            auto addSink(std::shared_ptr<ILogSink> sink) -> void {
                if (!sink) {
                    return;
                }
                std::shared_ptr<const SinkList> previous;
                const std::lock_guard lock{m_sinkMutex};
                previous = m_sinks.load();
                if (std::find(previous->begin(), previous->end(), sink) == previous->end()) {
                    auto next = std::make_shared<SinkList>(*previous);
                    next->push_back(std::move(sink));
                    m_sinks.store(std::move(next));
                }
            }

            auto removeSink(std::shared_ptr<ILogSink> sink) -> void {
                // Keep the old list alive until after unlocking, including when
                // removing the last reference to a sink with a custom destructor.
                std::shared_ptr<const SinkList> previous;
                const std::lock_guard lock{m_sinkMutex};
                previous = m_sinks.load();
                if (std::find(previous->begin(), previous->end(), sink) != previous->end()) {
                    auto next = std::make_shared<SinkList>(*previous);
                    std::erase(*next, sink);
                    m_sinks.store(std::move(next));
                }
            }

        private:
            using RecordPtr = std::shared_ptr<const LogRecord>;
            using SinkList = std::vector<std::shared_ptr<ILogSink>>;
            mutable std::mutex m_historyMutex;
            std::vector<RecordPtr> m_records;
            std::size_t m_start{};
            std::size_t m_size{};
            std::uint64_t m_nextSequence{1};
            std::mutex m_sinkMutex;
            // Copy the list only on subscription changes, not on every log.
            std::atomic<std::shared_ptr<const SinkList>> m_sinks{std::make_shared<const SinkList>()};
        };

        class BackendLogger final : public spdlog::logger {
        public:
            explicit BackendLogger(const std::shared_ptr<RecordSink>& sink) : spdlog::logger("Aether", sink) {
                // Allocation failures may propagate, as in the engine API.
                // User sink failures have already been isolated by RecordSink.
                set_error_handler([](const std::string&) {
                    if (const auto error = std::current_exception()) {
                        std::rethrow_exception(error);
                    }
                });
            }

            auto write(LogLevel level, std::string_view message, std::string_view category) -> void {
                const auto severity = detail::toSpdlogLevel(level);
                if (!should_log(severity)) {
                    return;
                }
                // Use spdlog's protected dispatch entry point to supply a category
                // per record without an unbounded cache of named loggers. Views
                // are consumed synchronously; arbitrary text is never a format string.
                const spdlog::details::log_msg event{
                    spdlog::source_loc{}, detail::toSpdlogView(category), severity, detail::toSpdlogView(message)};
                log_it_(event, true, false);
            }
        };
    }

    class Logger::Impl {
    public:
        explicit Impl(std::size_t capacity)
            : records(std::make_shared<RecordSink>(capacity)), backend(records) {}

        std::shared_ptr<RecordSink> records;
        BackendLogger backend;
    };

    Logger::Logger(std::size_t capacity) : m_impl(std::make_unique<Impl>(capacity)) {}
    Logger::~Logger() = default;

    auto Logger::log(LogLevel level, std::string_view message, std::string_view category) -> void {
        m_impl->backend.write(level, message, category);
    }

    auto Logger::trace(std::string_view message, std::string_view category) -> void { log(LogLevel::Trace, message, category); }
    auto Logger::debug(std::string_view message, std::string_view category) -> void { log(LogLevel::Debug, message, category); }
    auto Logger::info(std::string_view message, std::string_view category) -> void { log(LogLevel::Info, message, category); }
    auto Logger::warning(std::string_view message, std::string_view category) -> void { log(LogLevel::Warning, message, category); }
    auto Logger::error(std::string_view message, std::string_view category) -> void { log(LogLevel::Error, message, category); }
    auto Logger::fatal(std::string_view message, std::string_view category) -> void { log(LogLevel::Fatal, message, category); }

    auto Logger::setMinLevel(LogLevel level) -> void { m_impl->backend.set_level(detail::toSpdlogLevel(level)); }
    auto Logger::minLevel() const -> LogLevel { return detail::fromSpdlogLevel(m_impl->backend.level()); }
    auto Logger::setCapacity(std::size_t capacity) -> void { m_impl->records->setCapacity(capacity); }
    auto Logger::capacity() const -> std::size_t { return m_impl->records->capacity(); }
    auto Logger::snapshot() const -> std::vector<LogRecord> { return m_impl->records->snapshot(); }
    auto Logger::clear() -> void { m_impl->records->clear(); }
    auto Logger::addSink(std::shared_ptr<ILogSink> sink) -> void { m_impl->records->addSink(std::move(sink)); }
    auto Logger::removeSink(std::shared_ptr<ILogSink> sink) -> void { m_impl->records->removeSink(std::move(sink)); }
}