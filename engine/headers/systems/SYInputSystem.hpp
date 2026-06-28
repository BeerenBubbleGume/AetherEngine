//
// Created by drhaz on 18.06.2026.
//

#ifndef SMB_SINPUTSYSTEM_HPP
#define SMB_SINPUTSYSTEM_HPP

#include <unordered_map>

#include "interfaces/IUpdatable.hpp"

namespace engine::systems {

        enum class SYInputAction {
                MoveForward,
                MoveBackward,
                MoveLeft,
                MoveRight,
                MoveUp,
                MoveDown,
                Jump,
                Crouch,
                Sprint,
                Interact,
                Interact2,
                RotateLeft,
                RotateRight,
                LookUp,
                LookDown,
                LookLeft,
                LookRight,
                CameraMoveForward,
                CameraMoveBackward,
                CameraMoveLeft,
                CameraMoveRight,
                CameraMoveUp,
                Reset,
                Quit
        };

        enum class SYKey {
                A,
                B,
                C,
                D,
                E,
                F,
                G,
                H,
                I,
                J,
                K,
                L,
                M,
                N,
                O,
                P,
                Q,
                R,
                S,
                T,
                U,
                V,
                W,
                X,
                Y,
                Z,
                One,
                Two,
                Three,
                Four,
                Five,
                Six,
                Seven,
                Eight,
                Nine,
                Zero,
                Space,
                Escape,
                Enter,
                Backspace,
                Tab,
                Shift,
                Control,
                Alt,
                Left,
                Right,
                Up,
                Down,
                PageUp,
                PageDown,
                Home,
                End,
                Insert,
                Delete,
                F1,
                F2,
                F3,
                F4,
                F5,
                F6,
                F7,
                F8,
                F9,
                F10,
                F11,
                F12,
                NumPad0,
                NumPad1,
                NumPad2,
                NumPad3,
                NumPad4,
                NumPad5,
                NumPad6,
                NumPad7,
                NumPad8,
                NumPad9,
                NumPadAdd,
                NumPadSubtract,
                NumPadMultiply,
                NumPadDivide,
                NumPadDecimal,
                NumPadEnter,
                NumPadEqual,
                NumPadComma,
                NumPadMinus,
                NumPadPeriod,
                NumPadSlash,
                NumPadLeftBracket,
                NumPadRightBracket,
                NumPadBackslash,
                NumPadSemicolon,
                NumPadApostrophe,
                NumPadGrave,
                NumLock,
                CapsLock,
                Minus,
                Equal,
                LeftBracket,
                RightBracket,
                Backslash,
                Semicolon,
                Apostrophe,
                Grave,
                Comma,
                Period,
                Slash,
                Return,
        };

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
                auto processEvents(const SDL_Event &event) -> void;
                auto update(float delta) -> void override;
                auto bindKey(SYInputAction action, SYKey key) -> void;
        private:
                SYInputSystem() = default;
                ~SYInputSystem() override;

                [[nodiscard]] static auto toSdlKey(SYKey key) -> SDL_Scancode;
                [[nodiscard]] auto isKeyDown(SDL_Scancode key) const -> bool;

                std::array<bool, SDL_SCANCODE_COUNT> m_keyDown{};
                std::array<bool, SDL_SCANCODE_COUNT> m_keyPressed{};
                std::array<bool, SDL_SCANCODE_COUNT> m_keyReleased{};
                std::unordered_map<SYInputAction, std::vector<SYKey>> m_actionBindings;
        };
}
// systems

#endif //SMB_SINPUTSYSTEM_HPP
