//
// Created by drhaz on 18.06.2026.
//

#ifndef SMB_SINPUTSYSTEM_HPP
#define SMB_SINPUTSYSTEM_HPP

#include <optional>
#include <unordered_map>

#include "interfaces/IUpdatable.hpp"
#include "SYTypes.hpp"

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

                [[nodiscard]] auto isActionDown(SYInputAction key) const -> bool;
                [[nodiscard]] auto wasActionPressed(SYInputAction key) const -> bool;
                [[nodiscard]] auto wasActionReleased(SYInputAction key) const -> bool;
                [[nodiscard]] auto mouseX() const -> float;
                [[nodiscard]] auto mouseY() const -> float;
                [[nodiscard]] auto mouseDeltaX() const -> float;
                [[nodiscard]] auto mouseDeltaY() const -> float;
                [[nodiscard]] auto mouseWheelX() const -> float;
                [[nodiscard]] auto mouseWheelY() const -> float;

                [[nodiscard]] auto isMouseButtonDown(SYMouseButton button) const -> bool;
                [[nodiscard]] auto wasMouseButtonPressed(SYMouseButton button) const -> bool;
                [[nodiscard]] auto wasMouseButtonReleased(SYMouseButton button) const -> bool;
                auto processEvents(const SDL_Event &event) -> void;
                auto update(float delta) -> void override;
                auto bindKey(SYInputAction action, SYKey key) -> void;
        private:
                SYInputSystem() = default;
                ~SYInputSystem() override;

                [[nodiscard]] static auto toSdlKey(SYKey key) -> SDL_Scancode;
                [[nodiscard]] static auto toMouseIndex(SYMouseButton button) -> int;
                [[nodiscard]] auto isKeyDown(SDL_Scancode key) const -> bool;
                [[nodiscard]] static auto fromSdlMouseButtons(Uint8 button) -> std::optional<int>;

                std::array<bool, SDL_SCANCODE_COUNT> m_keyDown{};
                std::array<bool, SDL_SCANCODE_COUNT> m_keyPressed{};
                std::array<bool, SDL_SCANCODE_COUNT> m_keyReleased{};
                std::unordered_map<SYInputAction, std::vector<SYKey>> m_actionBindings;

                static constexpr size_t MouseButtonCount = 5;

                std::array<bool, MouseButtonCount> m_mouseDown{};
                std::array<bool, MouseButtonCount> m_mousePressed{};
                std::array<bool, MouseButtonCount> m_mouseReleased{};

                SYMouseState m_mouse{};
        };
}
// systems

#endif //SMB_SINPUTSYSTEM_HPP
