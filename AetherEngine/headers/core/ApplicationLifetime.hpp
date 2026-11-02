#pragma once

#include "IApplication.hpp"

namespace AetherEngine::core {
    // Created before init(): partial initialization must be cleaned up too.
    class ApplicationLifetime final {
    public:
        ApplicationLifetime(IApplication& app, EngineContext& ctx, EngineState& state)
            : m_app(app), m_ctx(ctx), m_state(state) {}
        ApplicationLifetime(const ApplicationLifetime&) = delete;
        auto operator=(const ApplicationLifetime&) -> ApplicationLifetime& = delete;

        auto shutdown() -> void {
            m_state = EngineState::Stopped;
            if (m_stopped) return;
            m_stopped = true;
            m_app.shutdown(m_ctx);
        }

        ~ApplicationLifetime() noexcept {
            try {
                shutdown();
            } catch (...) {
                // Never replace an initialization/run error during unwinding.
                try { m_ctx.logger.error("Application shutdown failed", "Engine"); } catch (...) {}
            }
        }

    private:
        IApplication& m_app;
        EngineContext& m_ctx;
        EngineState& m_state;
        bool m_stopped = false;
    };
}
