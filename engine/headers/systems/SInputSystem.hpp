//
// Created by drhaz on 18.06.2026.
//

#ifndef SMB_SINPUTSYSTEM_HPP
#define SMB_SINPUTSYSTEM_HPP
#include <expected>
#include <memory>
#include <unordered_set>

#include "ISystem.hpp"

namespace engine::systems {

        class SInputSystem : public systems::ISystem {
        public:
                struct SInputDeleter {
                        void operator()(SInputSystem* inputSystem) const {
                                delete inputSystem;
                        }
                };
                using SInputPtr = std::unique_ptr<SInputSystem, SInputDeleter>;

                static SInputPtr createInputSystem();

                [[nodiscard]] auto processEvents(const SDL_Event &event) -> std::expected<void, ISystemError>;
                auto update(float delta) -> void override;

        private:
                SInputSystem() = default;
                SInputSystem(const SInputSystem&) = delete;
                SInputSystem& operator=(const SInputSystem&) = delete;
                ~SInputSystem() override;

                std::unordered_set<SDL_Keycode> keysPressed;
        };
}
// systems

#endif //SMB_SINPUTSYSTEM_HPP
