//
// Created by drhaz on 22.06.2026.
//

#ifndef SMB_IAPPLICATION_HPP
#define SMB_IAPPLICATION_HPP

#include <expected>
#include "EngineContext.hpp"

namespace engine::core {
    class IApplication {
    public:
        virtual ~IApplication() = default;
        virtual auto init(EngineContext& ctx) -> std::expected<void, EngineError> = 0;
        virtual auto update(float dt, EngineContext& ctx) -> void = 0;
    };
} // interfaces
// engine

#endif //SMB_IAPPLICATION_HPP
