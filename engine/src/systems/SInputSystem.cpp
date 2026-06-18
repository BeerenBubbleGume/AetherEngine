//
// Created by drhaz on 18.06.2026.
//

#include "systems/SInputSystem.hpp"

namespace engine::systems {
    SInputSystem::SInputPtr SInputSystem::createInputSystem() {
        return SInputPtr(new SInputSystem(), SInputDeleter{});
    }
    auto SInputSystem::processEvents(const SDL_Event &event) -> std::expected<void, ISystemError> {
        switch (event.type) {
            case SDL_EVENT_KEY_DOWN:
                keysPressed.insert(event.key.scancode);
                break;
            case SDL_EVENT_KEY_UP:
                keysPressed.erase(event.key.scancode);
                break;

            default: break;
        }
        return {};
    }

    auto SInputSystem::update(float delta) -> void {
    }

    SInputSystem::~SInputSystem() = default;
}



