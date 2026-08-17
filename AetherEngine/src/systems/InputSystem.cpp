//
// Created by drhaz on 18.06.2026.
//

#include "systems/InputSystem.hpp"

#include <algorithm>

namespace AetherEngine::systems {
    namespace {
        auto isValidScancode(SDL_Scancode scancode) -> bool {
            const auto value = static_cast<int>(scancode);
            return value >= 0 && value < SDL_SCANCODE_COUNT;
        }
    }

    InputSystem::InputSystemPtr InputSystem::createInputSystem() {
        return InputSystemPtr(new InputSystem(), InputSystemDeleter{});
    }

    auto InputSystem::isActionDown(InputAction action) const -> bool {
        auto it = m_actionBindings.find(action);
        if (it == m_actionBindings.end()) {
            return false;
        }
        return std::ranges::any_of(it->second, [&](Key key) { return isKeyDown(toSdlKey(key)); });
    }

    auto InputSystem::processEvents(const SDL_Event &event) -> void {
        switch (event.type) {
            case SDL_EVENT_KEY_DOWN: {
                if (!isValidScancode(event.key.scancode)) {
                    break;
                }
                m_keyPressed[event.key.scancode] = true;
                if (m_keyReleased[event.key.scancode]) {
                    m_keyReleased[event.key.scancode] = false;
                }
                m_keyDown[event.key.scancode] = true;
                break;
            }
            case SDL_EVENT_KEY_UP: {
                if (!isValidScancode(event.key.scancode)) {
                    break;
                }
                m_keyPressed[event.key.scancode] = false;
                if (m_keyDown[event.key.scancode]) {
                    m_keyReleased[event.key.scancode] = true;
                }
                m_keyDown[event.key.scancode] = false;
                break;
            }
            case SDL_EVENT_MOUSE_MOTION: {
                m_mouse.x = event.motion.x;
                m_mouse.y = event.motion.y;
                m_mouse.deltaX += event.motion.xrel;
                m_mouse.deltaY += event.motion.yrel;
                break;
            }
            case SDL_EVENT_MOUSE_BUTTON_DOWN: {
                m_mouse.x = event.button.x;
                m_mouse.y = event.button.y;

                auto index = fromSdlMouseButtons(event.button.button);
                if (!index) break;

                if (!m_mouseDown[*index]) {
                    m_mousePressed[*index] = true;
                }

                m_mouseDown[*index] = true;
                break;
            }
            case SDL_EVENT_MOUSE_BUTTON_UP: {
                m_mouse.x = event.button.x;
                m_mouse.y = event.button.y;

                auto index = fromSdlMouseButtons(event.button.button);
                if (!index) break;

                if (m_mouseDown[*index]) {
                    m_mouseReleased[*index] = true;
                }

                m_mouseDown[*index] = false;
                break;
            }
            case SDL_EVENT_MOUSE_WHEEL:
                m_mouse.wheelX += event.wheel.x;
                m_mouse.wheelY += event.wheel.y;
                m_mouse.x = event.wheel.mouse_x;
                m_mouse.y = event.wheel.mouse_y;
                break;
            default: break;
        }
    }

    auto InputSystem::isKeyDown(SDL_Scancode key) const -> bool {
        return isValidScancode(key) && m_keyDown[static_cast<std::size_t>(key)];
    }

    auto InputSystem::fromSdlMouseButtons(Uint8 button) -> std::optional<int> {
        switch (button) {
            case SDL_BUTTON_LEFT: return 0;
            case SDL_BUTTON_MIDDLE: return 1;
            case SDL_BUTTON_RIGHT: return 2;
            case SDL_BUTTON_X1: return 3;
            case SDL_BUTTON_X2: return 4;
            default: return std::nullopt;
        }
    }

    auto InputSystem::toMouseIndex(MouseButton button) -> int {
        switch (button) {
            case MouseButton::Left: return 0;
            case MouseButton::Middle: return 1;
            case MouseButton::Right: return 2;
            case MouseButton::X1: return 3;
            case MouseButton::X2: return 4;
        }

        return 0;
    }

    auto InputSystem::wasActionPressed(InputAction action) const -> bool {
        auto it = m_actionBindings.find(action);
        if (it == m_actionBindings.end()) {
            return false;
        }

        return std::ranges::any_of(it->second, [&](Key key) {
            const auto scancode = toSdlKey(key);
            return scancode != SDL_SCANCODE_UNKNOWN && m_keyPressed[scancode];
        });
    }

    auto InputSystem::wasActionReleased(InputAction action) const -> bool {
        auto it = m_actionBindings.find(action);
        if (it == m_actionBindings.end()) {
            return false;
        }

        return std::ranges::any_of(it->second, [&](Key key) {
            const auto scancode = toSdlKey(key);
            return scancode != SDL_SCANCODE_UNKNOWN && m_keyReleased[scancode];
        });
    }

    auto InputSystem::mouseX() const -> float {
        return m_mouse.x;
    }

    auto InputSystem::mouseY() const -> float {
        return m_mouse.y;
    }

    auto InputSystem::mouseDeltaX() const -> float {
        return m_mouse.deltaX;
    }

    auto InputSystem::mouseDeltaY() const -> float {
        return m_mouse.deltaY;
    }

    auto InputSystem::mouseWheelX() const -> float {
        return m_mouse.wheelX;
    }

    auto InputSystem::mouseWheelY() const -> float {
        return m_mouse.wheelY;
    }

    auto InputSystem::isMouseButtonDown(MouseButton button) const -> bool {
        auto index = toMouseIndex(button);
        return m_mouseDown[index];
    }

    auto InputSystem::wasMouseButtonPressed(MouseButton button) const -> bool {
        auto index = toMouseIndex(button);
        return m_mousePressed[index];
    }

    auto InputSystem::wasMouseButtonReleased(MouseButton button) const -> bool {
        auto index = toMouseIndex(button);
        return m_mouseReleased[index];
    }

    auto InputSystem::update(float delta) -> void {
        m_keyPressed.fill(false);
        m_keyReleased.fill(false);

        m_mousePressed.fill(false);
        m_mouseReleased.fill(false);

        m_mouse.deltaX = 0.0f;
        m_mouse.deltaY = 0.0f;
        m_mouse.wheelX = 0.0f;
        m_mouse.wheelY = 0.0f;
    }

    auto InputSystem::bindKey(InputAction action, Key key) -> void {
        m_actionBindings[action].push_back(key);
    }

    InputSystem::~InputSystem() = default;

    auto InputSystem::toSdlKey(Key key) -> SDL_Scancode {
        switch (key) {
                case Key::A: return SDL_SCANCODE_A;
                case Key::B: return SDL_SCANCODE_B;
                case Key::C: return SDL_SCANCODE_C;
                case Key::D: return SDL_SCANCODE_D;
                case Key::E: return SDL_SCANCODE_E;
                case Key::F: return SDL_SCANCODE_F;
                case Key::G: return SDL_SCANCODE_G;
                case Key::H: return SDL_SCANCODE_H;
                case Key::I: return SDL_SCANCODE_I;
                case Key::J: return SDL_SCANCODE_J;
                case Key::K: return SDL_SCANCODE_K;
                case Key::L: return SDL_SCANCODE_L;
                case Key::M: return SDL_SCANCODE_M;
                case Key::N: return SDL_SCANCODE_N;
                case Key::O: return SDL_SCANCODE_O;
                case Key::P: return SDL_SCANCODE_P;
                case Key::Q: return SDL_SCANCODE_Q;
                case Key::R: return SDL_SCANCODE_R;
                case Key::S: return SDL_SCANCODE_S;
                case Key::T: return SDL_SCANCODE_T;
                case Key::U: return SDL_SCANCODE_U;
                case Key::V: return SDL_SCANCODE_V;
                case Key::W: return SDL_SCANCODE_W;
                case Key::X: return SDL_SCANCODE_X;
                case Key::Y: return SDL_SCANCODE_Y;
                case Key::Z: return SDL_SCANCODE_Z;
                case Key::One: return SDL_SCANCODE_1;
                case Key::Two: return SDL_SCANCODE_2;
                case Key::Three: return SDL_SCANCODE_3;
                case Key::Four: return SDL_SCANCODE_4;
                case Key::Five: return SDL_SCANCODE_5;
                case Key::Six: return SDL_SCANCODE_6;
                case Key::Seven: return SDL_SCANCODE_7;
                case Key::Eight: return SDL_SCANCODE_8;
                case Key::Nine: return SDL_SCANCODE_9;
                case Key::Zero: return SDL_SCANCODE_0;
                case Key::Space: return SDL_SCANCODE_SPACE;
                case Key::Escape: return SDL_SCANCODE_ESCAPE;
                case Key::Enter: return SDL_SCANCODE_RETURN;
                case Key::Backspace: return SDL_SCANCODE_BACKSPACE;
                case Key::Tab: return SDL_SCANCODE_TAB;
                case Key::Shift: return SDL_SCANCODE_LSHIFT;
                case Key::Control: return SDL_SCANCODE_LCTRL;
                case Key::Alt: return SDL_SCANCODE_LALT;
                case Key::Left: return SDL_SCANCODE_LEFT;
                case Key::Right: return SDL_SCANCODE_RIGHT;
                case Key::Up: return SDL_SCANCODE_UP;
                case Key::Down: return SDL_SCANCODE_DOWN;
                case Key::PageUp: return SDL_SCANCODE_PAGEUP;
                case Key::PageDown: return SDL_SCANCODE_PAGEDOWN;
                case Key::Home: return SDL_SCANCODE_HOME;
                case Key::End: return SDL_SCANCODE_END;
                case Key::Insert: return SDL_SCANCODE_INSERT;
                case Key::Delete: return SDL_SCANCODE_DELETE;
                case Key::F1: return SDL_SCANCODE_F1;
                case Key::F2: return SDL_SCANCODE_F2;
                case Key::F3: return SDL_SCANCODE_F3;
                case Key::F4: return SDL_SCANCODE_F4;
                case Key::F5: return SDL_SCANCODE_F5;
                case Key::F6: return SDL_SCANCODE_F6;
                case Key::F7: return SDL_SCANCODE_F7;
                case Key::F8: return SDL_SCANCODE_F8;
                case Key::F9: return SDL_SCANCODE_F9;
                case Key::F10: return SDL_SCANCODE_F10;
                case Key::F11: return SDL_SCANCODE_F11;
                case Key::F12: return SDL_SCANCODE_F12;
                case Key::NumPad0: return SDL_SCANCODE_KP_0;
                case Key::NumPad1: return SDL_SCANCODE_KP_1;
                case Key::NumPad2: return SDL_SCANCODE_KP_2;
                case Key::NumPad3: return SDL_SCANCODE_KP_3;
                case Key::NumPad4: return SDL_SCANCODE_KP_4;
                case Key::NumPad5: return SDL_SCANCODE_KP_5;
                case Key::NumPad6: return SDL_SCANCODE_KP_6;
                case Key::NumPad7: return SDL_SCANCODE_KP_7;
                case Key::NumPad8: return SDL_SCANCODE_KP_8;
                case Key::NumPad9: return SDL_SCANCODE_KP_9;
                case Key::NumPadAdd: return SDL_SCANCODE_KP_PLUS;
                case Key::NumPadSubtract: return SDL_SCANCODE_KP_MINUS;
                case Key::NumPadMultiply: return SDL_SCANCODE_KP_MULTIPLY;
                case Key::NumPadDivide: return SDL_SCANCODE_KP_DIVIDE;
                case Key::NumPadDecimal: return SDL_SCANCODE_KP_PERIOD;
                case Key::NumPadEnter: return SDL_SCANCODE_KP_ENTER;
                case Key::NumPadEqual: return SDL_SCANCODE_KP_EQUALS;
                case Key::NumPadComma: return SDL_SCANCODE_KP_COMMA;
                case Key::NumPadMinus: return SDL_SCANCODE_KP_MINUS;
                case Key::NumPadPeriod: return SDL_SCANCODE_KP_PERIOD;
                case Key::NumPadSlash: return SDL_SCANCODE_KP_DIVIDE;
                default: return SDL_SCANCODE_UNKNOWN;
        }
    }
}


