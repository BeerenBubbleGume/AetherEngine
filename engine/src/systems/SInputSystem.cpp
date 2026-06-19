//
// Created by drhaz on 18.06.2026.
//

#include "systems/SInputSystem.hpp"

namespace engine::systems {
    SInputSystem::SInputPtr SInputSystem::createInputSystem() {
        return SInputPtr(new SInputSystem(), SInputDeleter{});
    }
    auto SInputSystem::processEvents(const SDL_Event &event) -> void {
        switch (event.type) {
            case SDL_EVENT_KEY_DOWN:
                keysPressed[event.key.scancode] = true;
                SDL_Log("Key pressed: %d", event.key.scancode);
                break;
            case SDL_EVENT_KEY_UP:
                keysPressed[event.key.scancode] = false;
                SDL_Log("Key released: %d", event.key.scancode);
                break;

            default: break;
        }
    }

    auto SInputSystem::isKeyPressed(SDL_Scancode key) const -> bool {
        return keysPressed[key];
    }

    auto SInputSystem::update(float delta) -> void {

    }

    SInputSystem::~SInputSystem() = default;
}



