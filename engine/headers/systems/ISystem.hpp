//
// Created by drhaz on 18.06.2026.
//

#ifndef SMB_ISYSTEM_HPP
#define SMB_ISYSTEM_HPP
#include <expected>
#include <SDL3/SDL_events.h>


namespace engine::systems {
        struct ISystemError {
            int code;
            const char* message;
        };
        class ISystem {
            public:
            virtual ~ISystem() = default;
            virtual auto update(float delta) -> void = 0;

        };
} // namespace engine::systems

#endif //SMB_ISYSTEM_HPP
