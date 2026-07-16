//
// Created by drhaz on 22.06.2026.
//

#ifndef SMB_IAPPLICATION_HPP
#define SMB_IAPPLICATION_HPP

#include <expected>
#include <SDL3/SDL_events.h>

#include "EngineContext.hpp"

namespace engine::core {
    class IApplication {
    public:
        virtual ~IApplication() = default;
        virtual auto init(EngineContext& ctx) -> std::expected<void, EngineError> = 0;
        virtual auto onEvent(const SDL_Event& event) -> void { (void)event; }
        virtual auto update(float dt, EngineContext& ctx) -> void = 0;
        virtual auto render(EngineContext& ctx) -> void = 0;
        virtual auto shutdown(EngineContext& ctx) -> void { (void)ctx; }
        [[nodiscard]] virtual auto runConfig() const -> EngineRunConfig {return {};}
    };
} // interfaces
// engine

#endif //SMB_IAPPLICATION_HPP
