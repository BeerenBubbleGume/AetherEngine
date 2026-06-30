//
// Created by drhaz on 18.06.2026.
//

#include "systems/SYInputSystem.hpp"

#include <algorithm>

namespace engine::systems {
    SYInputSystem::SInputPtr SYInputSystem::createInputSystem() {
        return SInputPtr(new SYInputSystem(), SInputDeleter{});
    }

    auto SYInputSystem::isActionDown(SYInputAction action) const -> bool {
        auto it = m_actionBindings.find(action);
        if (it == m_actionBindings.end()) {
            return false;
        }
        return std::ranges::any_of(it->second, [&](SYKey key) { return isKeyDown(toSdlKey(key)); });
    }

    auto SYInputSystem::processEvents(const SDL_Event &event) -> void {
        switch (event.type) {
            case SDL_EVENT_KEY_DOWN: {
                m_keyPressed[event.key.scancode] = true;
                if (m_keyReleased[event.key.scancode]) {
                    m_keyReleased[event.key.scancode] = false;
                }
                m_keyDown[event.key.scancode] = true;
                break;
            }
            case SDL_EVENT_KEY_UP: {
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

    auto SYInputSystem::isKeyDown(SDL_Scancode key) const -> bool {
        return m_keyDown[key];
    }

    auto SYInputSystem::fromSdlMouseButtons(Uint8 button) -> std::optional<int> {
        switch (button) {
            case SDL_BUTTON_LEFT: return 0;
            case SDL_BUTTON_MIDDLE: return 1;
            case SDL_BUTTON_RIGHT: return 2;
            case SDL_BUTTON_X1: return 3;
            case SDL_BUTTON_X2: return 4;
            default: return std::nullopt;
        }
    }

    auto SYInputSystem::toMouseIndex(SYMouseButton button) -> int {
        switch (button) {
            case SYMouseButton::Left: return 0;
            case SYMouseButton::Middle: return 1;
            case SYMouseButton::Right: return 2;
            case SYMouseButton::X1: return 3;
            case SYMouseButton::X2: return 4;
        }

        return 0;
    }

    auto SYInputSystem::wasActionPressed(SYInputAction action) const -> bool {
        auto it = m_actionBindings.find(action);
        if (it == m_actionBindings.end()) {
            return false;
        }

        return std::ranges::any_of(it->second, [&](SYKey key) {
            const auto scancode = toSdlKey(key);
            return scancode != SDL_SCANCODE_UNKNOWN && m_keyPressed[scancode];
        });
    }

    auto SYInputSystem::wasActionReleased(SYInputAction action) const -> bool {
        auto it = m_actionBindings.find(action);
        if (it == m_actionBindings.end()) {
            return false;
        }

        return std::ranges::any_of(it->second, [&](SYKey key) {
            const auto scancode = toSdlKey(key);
            return scancode != SDL_SCANCODE_UNKNOWN && m_keyReleased[scancode];
        });
    }

    auto SYInputSystem::mouseX() const -> float {
        return m_mouse.x;
    }

    auto SYInputSystem::mouseY() const -> float {
        return m_mouse.y;
    }

    auto SYInputSystem::mouseDeltaX() const -> float {
        return m_mouse.deltaX;
    }

    auto SYInputSystem::mouseDeltaY() const -> float {
        return m_mouse.deltaY;
    }

    auto SYInputSystem::mouseWheelX() const -> float {
        return m_mouse.wheelX;
    }

    auto SYInputSystem::mouseWheelY() const -> float {
        return m_mouse.wheelY;
    }

    auto SYInputSystem::isMouseButtonDown(SYMouseButton button) const -> bool {
        auto index = toMouseIndex(button);
        return m_mouseDown[index];
    }

    auto SYInputSystem::wasMouseButtonPressed(SYMouseButton button) const -> bool {
        auto index = toMouseIndex(button);
        return m_mousePressed[index];
    }

    auto SYInputSystem::wasMouseButtonReleased(SYMouseButton button) const -> bool {
        auto index = toMouseIndex(button);
        return m_mouseReleased[index];
    }

    auto SYInputSystem::update(float delta) -> void {
        m_keyPressed.fill(false);
        m_keyReleased.fill(false);

        m_mousePressed.fill(false);
        m_mouseReleased.fill(false);

        m_mouse.deltaX = 0.0f;
        m_mouse.deltaY = 0.0f;
        m_mouse.wheelX = 0.0f;
        m_mouse.wheelY = 0.0f;
    }

    auto SYInputSystem::bindKey(SYInputAction action, SYKey key) -> void {
        m_actionBindings[action].push_back(key);
    }

    SYInputSystem::~SYInputSystem() = default;

    auto SYInputSystem::toSdlKey(SYKey key) -> SDL_Scancode {
        switch (key) {
                case SYKey::A: return SDL_SCANCODE_A;
                case SYKey::B: return SDL_SCANCODE_B;
                case SYKey::C: return SDL_SCANCODE_C;
                case SYKey::D: return SDL_SCANCODE_D;
                case SYKey::E: return SDL_SCANCODE_E;
                case SYKey::F: return SDL_SCANCODE_F;
                case SYKey::G: return SDL_SCANCODE_G;
                case SYKey::H: return SDL_SCANCODE_H;
                case SYKey::I: return SDL_SCANCODE_I;
                case SYKey::J: return SDL_SCANCODE_J;
                case SYKey::K: return SDL_SCANCODE_K;
                case SYKey::L: return SDL_SCANCODE_L;
                case SYKey::M: return SDL_SCANCODE_M;
                case SYKey::N: return SDL_SCANCODE_N;
                case SYKey::O: return SDL_SCANCODE_O;
                case SYKey::P: return SDL_SCANCODE_P;
                case SYKey::Q: return SDL_SCANCODE_Q;
                case SYKey::R: return SDL_SCANCODE_R;
                case SYKey::S: return SDL_SCANCODE_S;
                case SYKey::T: return SDL_SCANCODE_T;
                case SYKey::U: return SDL_SCANCODE_U;
                case SYKey::V: return SDL_SCANCODE_V;
                case SYKey::W: return SDL_SCANCODE_W;
                case SYKey::X: return SDL_SCANCODE_X;
                case SYKey::Y: return SDL_SCANCODE_Y;
                case SYKey::Z: return SDL_SCANCODE_Z;
                case SYKey::One: return SDL_SCANCODE_1;
                case SYKey::Two: return SDL_SCANCODE_2;
                case SYKey::Three: return SDL_SCANCODE_3;
                case SYKey::Four: return SDL_SCANCODE_4;
                case SYKey::Five: return SDL_SCANCODE_5;
                case SYKey::Six: return SDL_SCANCODE_6;
                case SYKey::Seven: return SDL_SCANCODE_7;
                case SYKey::Eight: return SDL_SCANCODE_8;
                case SYKey::Nine: return SDL_SCANCODE_9;
                case SYKey::Zero: return SDL_SCANCODE_0;
                case SYKey::Space: return SDL_SCANCODE_SPACE;
                case SYKey::Escape: return SDL_SCANCODE_ESCAPE;
                case SYKey::Enter: return SDL_SCANCODE_RETURN;
                case SYKey::Backspace: return SDL_SCANCODE_BACKSPACE;
                case SYKey::Tab: return SDL_SCANCODE_TAB;
                case SYKey::Shift: return SDL_SCANCODE_LSHIFT;
                case SYKey::Control: return SDL_SCANCODE_LCTRL;
                case SYKey::Alt: return SDL_SCANCODE_LALT;
                case SYKey::Left: return SDL_SCANCODE_LEFT;
                case SYKey::Right: return SDL_SCANCODE_RIGHT;
                case SYKey::Up: return SDL_SCANCODE_UP;
                case SYKey::Down: return SDL_SCANCODE_DOWN;
                case SYKey::PageUp: return SDL_SCANCODE_PAGEUP;
                case SYKey::PageDown: return SDL_SCANCODE_PAGEDOWN;
                case SYKey::Home: return SDL_SCANCODE_HOME;
                case SYKey::End: return SDL_SCANCODE_END;
                case SYKey::Insert: return SDL_SCANCODE_INSERT;
                case SYKey::Delete: return SDL_SCANCODE_DELETE;
                case SYKey::F1: return SDL_SCANCODE_F1;
                case SYKey::F2: return SDL_SCANCODE_F2;
                case SYKey::F3: return SDL_SCANCODE_F3;
                case SYKey::F4: return SDL_SCANCODE_F4;
                case SYKey::F5: return SDL_SCANCODE_F5;
                case SYKey::F6: return SDL_SCANCODE_F6;
                case SYKey::F7: return SDL_SCANCODE_F7;
                case SYKey::F8: return SDL_SCANCODE_F8;
                case SYKey::F9: return SDL_SCANCODE_F9;
                case SYKey::F10: return SDL_SCANCODE_F10;
                case SYKey::F11: return SDL_SCANCODE_F11;
                case SYKey::F12: return SDL_SCANCODE_F12;
                case SYKey::NumPad0: return SDL_SCANCODE_KP_0;
                case SYKey::NumPad1: return SDL_SCANCODE_KP_1;
                case SYKey::NumPad2: return SDL_SCANCODE_KP_2;
                case SYKey::NumPad3: return SDL_SCANCODE_KP_3;
                case SYKey::NumPad4: return SDL_SCANCODE_KP_4;
                case SYKey::NumPad5: return SDL_SCANCODE_KP_5;
                case SYKey::NumPad6: return SDL_SCANCODE_KP_6;
                case SYKey::NumPad7: return SDL_SCANCODE_KP_7;
                case SYKey::NumPad8: return SDL_SCANCODE_KP_8;
                case SYKey::NumPad9: return SDL_SCANCODE_KP_9;
                case SYKey::NumPadAdd: return SDL_SCANCODE_KP_PLUS;
                case SYKey::NumPadSubtract: return SDL_SCANCODE_KP_MINUS;
                case SYKey::NumPadMultiply: return SDL_SCANCODE_KP_MULTIPLY;
                case SYKey::NumPadDivide: return SDL_SCANCODE_KP_DIVIDE;
                case SYKey::NumPadDecimal: return SDL_SCANCODE_KP_PERIOD;
                case SYKey::NumPadEnter: return SDL_SCANCODE_KP_ENTER;
                case SYKey::NumPadEqual: return SDL_SCANCODE_KP_EQUALS;
                case SYKey::NumPadComma: return SDL_SCANCODE_KP_COMMA;
                case SYKey::NumPadMinus: return SDL_SCANCODE_KP_MINUS;
                case SYKey::NumPadPeriod: return SDL_SCANCODE_KP_PERIOD;
                case SYKey::NumPadSlash: return SDL_SCANCODE_KP_DIVIDE;
                default: return SDL_SCANCODE_UNKNOWN;
        }
    }
}



