//
// Created by drhaz on 18.06.2026.
//

#ifndef SMB_ISYSTEM_HPP
#define SMB_ISYSTEM_HPP
#include <expected>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_log.h>
#include <array>
#include <expected>
#include <memory>
#include <string>

namespace AetherEngine::systems {
        struct IUpdatableError {
            int code;
            std::string message;
        };
        class IUpdatable {
            public:
            virtual ~IUpdatable() = default;
            virtual auto update(float delta) -> void = 0;

        };
} // namespace AetherEngine::systems

#endif //SMB_ISYSTEM_HPP
