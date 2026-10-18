#include <algorithm>
#include <atomic>
#include <iostream>
#include <latch>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/null_sink.h>

#include "core/EngineContext.hpp"
#include "logging/Logger.hpp"

namespace {
    using namespace AetherEngine::logging;
    static_assert(std::is_same_v<decltype(AetherEngine::core::EngineContext::logger), Logger&>);

    auto expect(bool condition, const char* message) -> void {
        if (!condition) {
            throw std::runtime_error{message};
        }
    }

    class RecordingSink final : public ILogSink {
    public:
        auto write(const LogRecord& record) -> void override {
            const std::lock_guard lock{mutex};
            records.push_back(record);
        }

        std::mutex mutex;
        std::vector<LogRecord> records;
    };

    auto boundedHistory() -> void {
        Logger logger{3};
        for (int i = 1; i <= 5; ++i) {
            logger.info(std::to_string(i), "Test");
        }
        auto records = logger.snapshot();
        expect(logger.capacity() == 3 && records.size() == 3, "history respects capacity");
        expect(records.front().message == "3" && records.back().message == "5", "oldest records are evicted");
        expect(records.front().sequence == 3 && records.back().sequence == 5, "history is ordered");
        logger.setCapacity(2);
        expect(logger.snapshot().front().message == "4", "shrinking retains newest records");
        logger.setCapacity(5);
        logger.info("6");
        records = logger.snapshot();
        expect(records.size() == 3 && records.front().message == "4" && records.back().message == "6",
            "growing preserves history and insertion order");
        logger.clear();
        expect(logger.snapshot().empty(), "clear empties history");
        expect(records.size() == 3 && records.front().message == "4", "snapshots own independent records");
        logger.info("7");
        expect(logger.snapshot().front().sequence == 7, "clear preserves sequence numbering");

        logger.setCapacity(0);
        auto sink = std::make_shared<RecordingSink>();
        logger.addSink(sink);
        logger.info("8");
        expect(logger.snapshot().empty() && logger.capacity() == 0, "zero capacity disables history");
        expect(sink->records.size() == 1 && sink->records.front().sequence == 8, "zero capacity still delivers to sinks");
        logger.setCapacity(1);
        logger.info("9");
        logger.info("10");
        expect(logger.snapshot().size() == 1 && logger.snapshot().front().message == "10", "one-slot ring wraps");
        Logger disabled{0};
        disabled.info("discarded history");
        disabled.clear();
        disabled.setCapacity(1);
        disabled.info("retained");
        expect(disabled.snapshot().front().sequence == 2, "initially disabled history can be enabled");
    }

    auto recordMetadataAndBounds() -> void {
        Logger logger;
        std::string text = "owned message";
        std::string category = "owned category";
        const auto before = std::chrono::system_clock::now();
        logger.warning(text, category);
        const auto after = std::chrono::system_clock::now();
        text.assign("changed");
        category.assign("changed");
        auto record = logger.snapshot().front();
        expect(record.message == "owned message" && record.category == "owned category", "records own their text");
        expect(record.level == LogLevel::Warning && !record.truncated, "record preserves level");
        expect(record.timestamp >= before && record.timestamp <= after, "record contains wall-clock timestamp");
        expect(record.threadId == std::this_thread::get_id(), "record identifies logging thread");

        const std::string longMessage(Logger::MaxMessageBytes + 500, 'x');
        const std::string longCategory(Logger::MaxCategoryBytes + 100, 'c');
        logger.error(longMessage, longCategory);
        record = logger.snapshot().back();
        expect(record.message.size() == Logger::MaxMessageBytes && record.category.size() == Logger::MaxCategoryBytes,
            "message and category allocations are bounded");
        expect(record.truncated, "truncation is visible");
        const std::string utf8 = "\xe2\x82\xac";
        logger.info(std::string(Logger::MaxMessageBytes - 1, 'x') + utf8,
            std::string(Logger::MaxCategoryBytes - 1, 'c') + utf8);
        record = logger.snapshot().back();
        expect(record.message.size() == Logger::MaxMessageBytes - 1 && record.category.size() == Logger::MaxCategoryBytes - 1,
            "truncation preserves UTF-8 code point boundaries");
        logger.info(std::string(Logger::MaxMessageBytes, 'x'));
        expect(!logger.snapshot().back().truncated, "exact byte limit is not truncation");
    }

    auto levelsAndSinks() -> void {
        Logger logger;
        auto sink = std::make_shared<RecordingSink>();
        logger.addSink(nullptr);
        logger.addSink(sink);
        logger.addSink(sink);
        expect(logger.minLevel() == LogLevel::Info, "default minimum level is Info");
        logger.trace("hidden trace");
        logger.debug("hidden debug");
        expect(logger.snapshot().empty() && sink->records.empty(), "filter applies to history and sinks");
        logger.setMinLevel(LogLevel::Trace);
        logger.trace("trace");
        logger.debug("debug");
        logger.info("info");
        logger.warning("warning");
        logger.error("error");
        logger.fatal("fatal");
        const auto records = logger.snapshot();
        expect(records.size() == 6 && sink->records.size() == 6, "all levels are delivered once per sink");
        for (std::size_t i = 0; i < records.size(); ++i) {
            expect(records[i].level == static_cast<LogLevel>(i) && records[i].sequence == i + 1,
                "convenience methods preserve levels and filtering consumes no sequence numbers");
            expect(sink->records[i].sequence == records[i].sequence, "sink sees the buffered record");
        }
        logger.setMinLevel(LogLevel::Error);
        logger.warning("filtered warning");
        expect(sink->records.size() == 6, "updated threshold is respected");
        logger.removeSink(sink);
        logger.removeSink(sink);
        logger.removeSink(nullptr);
        logger.error("after removal");
        expect(sink->records.size() == 6 && logger.snapshot().size() == 7, "removed sink stops receiving new records");
    }

    auto literalText() -> void {
        Logger logger;
        auto sink = std::make_shared<RecordingSink>();
        logger.addSink(sink);
        const std::string message{"literal {braces} %s\0tail", 24};
        const std::string category{"cat\0suffix", 10};
        logger.info(message, category);
        const auto records = logger.snapshot();
        expect(records.size() == 1 && records[0].message == message && records[0].category == category,
            "backend preserves literal format characters and embedded null bytes");
        expect(sink->records.size() == 1 && sink->records[0].message == message,
            "sink sees the same unformatted text");
        logger.info(std::string_view{message.data(), 7}, std::string_view{category.data(), 3});
        expect(logger.snapshot().back().message == "literal", "backend respects string_view length");
        logger.info({}, {});
        expect(logger.snapshot().back().message.empty() && logger.snapshot().back().category.empty(),
            "default empty string views are accepted");
    }

    class LifetimeSink final : public ILogSink {
    public:
        explicit LifetimeSink(int& destructions) : destructions(destructions) {}
        ~LifetimeSink() override { ++destructions; }
        auto write(const LogRecord&) -> void override {}
        int& destructions;
    };

    auto raiiAndRegistryIsolation() -> void {
        const auto originalDefault = spdlog::default_logger();
        const auto originalLevel = originalDefault->level();
        auto foreign = std::make_shared<spdlog::logger>("Aether", std::make_shared<spdlog::sinks::null_sink_mt>());
        spdlog::register_logger(foreign);
        struct RegistryEntry {
            ~RegistryEntry() { spdlog::drop("Aether"); }
        } cleanup;

        int destructions = 0;
        std::weak_ptr<ILogSink> weakSink;
        std::vector<LogRecord> retained;
        {
            Logger first{2};
            Logger second{1};
            auto ownedSink = std::make_shared<LifetimeSink>(destructions);
            weakSink = ownedSink;
            first.addSink(std::move(ownedSink));
            expect(!weakSink.expired(), "logger owns its sinks after caller releases them");
            first.setMinLevel(LogLevel::Trace);
            first.debug("first", "SharedCategory");
            second.debug("filtered", "SharedCategory");
            second.info("second", "SharedCategory");
            retained = first.snapshot();
            expect(retained.size() == 1 && retained[0].sequence == 1 && retained[0].message == "first",
                "first logger has independent history and numbering");
            expect(second.snapshot().size() == 1 && second.snapshot()[0].sequence == 1 &&
                second.snapshot()[0].message == "second" && second.minLevel() == LogLevel::Info,
                "second logger has independent filter and history");
        }
        expect(weakSink.expired() && destructions == 1, "RAII destruction releases registered sinks exactly once");
        expect(retained[0].message == "first", "snapshot survives destruction of its logger");
        expect(spdlog::get("Aether") == foreign, "wrapper neither registers nor shuts down global spdlog loggers");
        expect(spdlog::default_logger() == originalDefault && originalDefault->level() == originalLevel,
            "wrapper preserves the host's default logger and configuration");
        Logger recreated;
        recreated.info("fresh");
        expect(recreated.snapshot()[0].sequence == 1, "logger can be recreated without global cleanup");
    }

    class ThrowingSink final : public ILogSink {
        auto write(const LogRecord&) -> void override {
            throw std::runtime_error{"sink failed"};
        }
    };

    class NonStdThrowingSink final : public ILogSink {
        auto write(const LogRecord&) -> void override { throw 42; }
    };

    class ReentrantSink final : public ILogSink, public std::enable_shared_from_this<ReentrantSink> {
    public:
        explicit ReentrantSink(Logger& logger) : logger(logger) {}
        auto write(const LogRecord&) -> void override {
            observed = logger.snapshot().size();
            logger.removeSink(shared_from_this());
            logger.info("nested");
        }
        Logger& logger;
        std::size_t observed{};
    };

    auto sinkFailuresAndReentry() -> void {
        Logger logger;
        auto reentrant = std::make_shared<ReentrantSink>(logger);
        auto recording = std::make_shared<RecordingSink>();
        logger.addSink(std::make_shared<ThrowingSink>());
        logger.addSink(std::make_shared<NonStdThrowingSink>());
        logger.addSink(reentrant);
        logger.addSink(recording);
        logger.info("outer");
        const auto records = logger.snapshot();
        expect(reentrant->observed == 1, "sink can inspect committed history without deadlock");
        expect(records.size() == 2 && records[0].message == "outer" && records[1].message == "nested",
            "sink can remove itself and log recursively without deadlock");
        expect(recording->records.size() == 2, "throwing sink does not prevent delivery to other sinks");
    }

    struct BlockingState {
        std::latch entered{1};
        std::latch release{1};
        std::atomic<bool> destroyed{};
    };

    class BlockingSink final : public ILogSink {
    public:
        explicit BlockingSink(std::shared_ptr<BlockingState> state) : state(std::move(state)) {}
        ~BlockingSink() override { state->destroyed = true; }
        auto write(const LogRecord&) -> void override {
            state->entered.count_down();
            state->release.wait();
        }
        std::shared_ptr<BlockingState> state;
    };

    auto inFlightSinkLifetime() -> void {
        Logger logger;
        auto state = std::make_shared<BlockingState>();
        auto sink = std::make_shared<BlockingSink>(state);
        logger.addSink(sink);
        std::jthread producer{[&] { logger.info("in flight"); }};
        state->entered.wait();
        logger.removeSink(sink);
        sink.reset();
        const bool retained = !state->destroyed.load();
        const bool readable = logger.snapshot().size() == 1;
        state->release.count_down();
        producer.join();
        expect(retained && readable, "removal keeps in-flight sink alive without blocking history access");
        expect(state->destroyed, "sink is released when its in-flight write finishes");
    }

    auto concurrentOperations() -> void {
        constexpr int ThreadCount = 8;
        constexpr int MessagesPerThread = 1000;
        constexpr std::size_t Capacity = 127;
        Logger logger{Capacity};
        auto sink = std::make_shared<RecordingSink>();
        auto temporarySink = std::make_shared<RecordingSink>();
        logger.addSink(sink);
        std::latch start{1};
        std::atomic<int> remaining{ThreadCount};
        std::atomic<bool> validSnapshots{true};
        std::vector<std::jthread> producers;
        for (int thread = 0; thread < ThreadCount; ++thread) {
            producers.emplace_back([&, thread] {
                start.wait();
                for (int i = 0; i < MessagesPerThread; ++i) {
                    logger.info(std::to_string(thread) + ":" + std::to_string(i), "Worker");
                }
                --remaining;
            });
        }
        std::jthread observer{[&] {
            start.wait();
            std::size_t iteration = 0;
            do {
                const auto records = logger.snapshot();
                if (records.size() > Capacity) {
                    validSnapshots = false;
                }
                for (std::size_t i = 1; i < records.size(); ++i) {
                    if (records[i - 1].sequence >= records[i].sequence || records[i].category != "Worker") {
                        validSnapshots = false;
                    }
                }
                logger.setCapacity(iteration++ % 2 == 0 ? Capacity : 0);
                logger.setMinLevel(LogLevel::Info);
                logger.addSink(temporarySink);
                logger.removeSink(temporarySink);
                logger.clear();
                std::this_thread::yield();
            } while (remaining.load() > 0);
        }};
        start.count_down();
        for (auto& producer : producers) {
            producer.join();
        }
        observer.join();
        expect(validSnapshots, "concurrent snapshots remain bounded and ordered during resize and clear");
        auto records = sink->records;
        expect(records.size() == ThreadCount * MessagesPerThread, "sink receives every concurrent write");
        std::sort(records.begin(), records.end(), [](const auto& left, const auto& right) {
            return left.sequence < right.sequence;
        });
        std::vector<std::string> messages;
        for (std::size_t i = 0; i < records.size(); ++i) {
            expect(records[i].sequence == i + 1, "concurrent sequence numbers are unique and gap-free");
            expect(records[i].threadId != std::this_thread::get_id() && records[i].threadId != std::thread::id{},
                "concurrent records identify worker threads");
            messages.push_back(records[i].message);
        }
        std::sort(messages.begin(), messages.end());
        expect(std::adjacent_find(messages.begin(), messages.end()) == messages.end(), "concurrent messages are not duplicated");
    }
}

int main() {
    int failures = 0;
    const auto run = [&](const char* name, auto test) {
        try {
            test();
            std::cout << "PASS: " << name << '\n';
        } catch (const std::exception& error) {
            std::cerr << "FAIL: " << name << ": " << error.what() << '\n';
            ++failures;
        }
    };
    run("bounded history", boundedHistory);
    run("record metadata and text bounds", recordMetadataAndBounds);
    run("levels and sink registration", levelsAndSinks);
    run("literal and binary text", literalText);
    run("RAII and registry isolation", raiiAndRegistryIsolation);
    run("sink failures and reentry", sinkFailuresAndReentry);
    run("in-flight sink lifetime", inFlightSinkLifetime);
    run("concurrent operations", concurrentOperations);
    return failures == 0 ? 0 : 1;
}
