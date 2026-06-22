//
// Created by drhaz on 18.06.2026.
//

#ifndef SMB_SINPUTSYSTEM_HPP
#define SMB_SINPUTSYSTEM_HPP

#include "interfaces/IUpdatable.hpp"

namespace engine::systems {

        class SYInputSystem : public systems::IUpdatable {
        public:
                SYInputSystem(const SYInputSystem&) = delete;
                SYInputSystem& operator=(const SYInputSystem&) = delete;
                SYInputSystem(SYInputSystem&&) = delete;
                SYInputSystem& operator=(SYInputSystem&&) = delete;

                struct SInputDeleter {
                        void operator()(SYInputSystem* inputSystem) const {
                                delete inputSystem;
                        }
                };
                using SInputPtr = std::unique_ptr<SYInputSystem, SInputDeleter>;

                static SInputPtr createInputSystem();

                [[nodiscard]] auto isKeyPressed(SDL_Scancode key) const -> bool;
                auto processEvents(const SDL_Event &event) -> void;
                auto update(float delta) -> void override;

        private:
                SYInputSystem() = default;
                ~SYInputSystem() override;

                std::array<bool, SDL_SCANCODE_COUNT> keysPressed{};
        };
}
// systems

#endif //SMB_SINPUTSYSTEM_HPP
