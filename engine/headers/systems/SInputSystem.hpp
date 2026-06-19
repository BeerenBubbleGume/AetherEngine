//
// Created by drhaz on 18.06.2026.
//

#ifndef SMB_SINPUTSYSTEM_HPP
#define SMB_SINPUTSYSTEM_HPP

#include "interfaces/IUpdatable.hpp"

namespace engine::systems {

        class SInputSystem : public systems::IUpdatable {
        public:
                SInputSystem(const SInputSystem&) = delete;
                SInputSystem& operator=(const SInputSystem&) = delete;
                SInputSystem(SInputSystem&&) = delete;
                SInputSystem& operator=(SInputSystem&&) = delete;

                struct SInputDeleter {
                        void operator()(SInputSystem* inputSystem) const {
                                delete inputSystem;
                        }
                };
                using SInputPtr = std::unique_ptr<SInputSystem, SInputDeleter>;

                static SInputPtr createInputSystem();

                [[nodiscard]] auto isKeyPressed(SDL_Scancode key) const -> bool;
                auto processEvents(const SDL_Event &event) -> void;
                auto update(float delta) -> void override;

        private:
                SInputSystem() = default;
                ~SInputSystem() override;

                std::array<bool, SDL_SCANCODE_COUNT> keysPressed{};
        };
}
// systems

#endif //SMB_SINPUTSYSTEM_HPP
