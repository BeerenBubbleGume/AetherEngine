#ifndef AETHERENGINE_LOGGING_ILOGSINK_HPP
#define AETHERENGINE_LOGGING_ILOGSINK_HPP

#include "LogRecord.hpp"

namespace AetherEngine::logging {
    class ILogSink {
    public:
        virtual ~ILogSink() = default;

        // Called synchronously on the logging thread, without the logger's lock.
        // Implementations must support concurrent calls. Delivery between threads
        // may be out of sequence; record.sequence gives the buffer insertion order.
        // Copy the record if it is needed after write() returns. Exceptions are
        // isolated by Logger so that other sinks still receive the record.
        virtual auto write(const LogRecord& record) -> void = 0;
    };
}

#endif
