//
// Created by drhaz on 18.06.2026.
//

#ifndef SMB_INPUTSYSTEM_HPP
#define SMB_INPUTSYSTEM_HPP

#include <optional>
#include <unordered_map>

#include "interfaces/IUpdatable.hpp"
#include "InputTypes.hpp"

namespace engine::systems {

        class InputSystem : public systems::IUpdatable {
        public:
                InputSystem(const InputSystem&) = delete;
                InputSystem& operator=(const InputSystem&) = delete;
                InputSystem(InputSystem&&) = delete;
                InputSystem& operator=(InputSystem&&) = delete;

                struct InputSystemDeleter {
                        void operator()(InputSystem* inputSystem) const {
                                delete inputSystem;
                        }
                };
                using InputSystemPtr = std::unique_ptr<InputSystem, InputSystemDeleter>;

                static InputSystemPtr createInputSystem();

                [[nodiscard]] auto isActionDown(InputAction key) const -> bool;
                [[nodiscard]] auto wasActionPressed(InputAction key) const -> bool;
                [[nodiscard]] auto wasActionReleased(InputAction key) const -> bool;
                [[nodiscard]] auto mouseX() const -> float;
                [[nodiscard]] auto mouseY() const -> float;
                [[nodiscard]] auto mouseDeltaX() const -> float;
                [[nodiscard]] auto mouseDeltaY() const -> float;
                [[nodiscard]] auto mouseWheelX() const -> float;
                [[nodiscard]] auto mouseWheelY() const -> float;

                [[nodiscard]] auto isMouseButtonDown(MouseButton button) const -> bool;
                [[nodiscard]] auto wasMouseButtonPressed(MouseButton button) const -> bool;
                [[nodiscard]] auto wasMouseButtonReleased(MouseButton button) const -> bool;
                auto processEvents(const SDL_Event &event) -> void;
                auto update(float delta) -> void override;
                auto bindKey(InputAction action, Key key) -> void;
        private:
                InputSystem() = default;
                ~InputSystem() override;

                [[nodiscard]] static auto toSdlKey(Key key) -> SDL_Scancode;
                [[nodiscard]] static auto toMouseIndex(MouseButton button) -> int;
                [[nodiscard]] auto isKeyDown(SDL_Scancode key) const -> bool;
                [[nodiscard]] static auto fromSdlMouseButtons(Uint8 button) -> std::optional<int>;

                std::array<bool, SDL_SCANCODE_COUNT> m_keyDown{};
                std::array<bool, SDL_SCANCODE_COUNT> m_keyPressed{};
                std::array<bool, SDL_SCANCODE_COUNT> m_keyReleased{};
                std::unordered_map<InputAction, std::vector<Key>> m_actionBindings;

                static constexpr size_t MouseButtonCount = 5;

                std::array<bool, MouseButtonCount> m_mouseDown{};
                std::array<bool, MouseButtonCount> m_mousePressed{};
                std::array<bool, MouseButtonCount> m_mouseReleased{};

                MouseState m_mouse{};
        };
}
// systems

#endif //SMB_INPUTSYSTEM_HPP
