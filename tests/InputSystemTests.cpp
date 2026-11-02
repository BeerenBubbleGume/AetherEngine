#include <iostream>
#include <SDL3/SDL_events.h>
#include "systems/InputSystem.hpp"

int main() {
    using namespace AetherEngine::systems;
    auto input = InputSystem::createInputSystem();
    input->bindKey(InputAction::Jump, Key::Space);
    const auto send = [&](Uint32 type, bool repeat = false) {
        SDL_Event event{};
        event.type = type;
        event.key.scancode = SDL_SCANCODE_SPACE;
        event.key.repeat = repeat;
        input->processEvents(event);
    };
    int failures = 0;
    const auto check = [&](bool ok, const char* message) {
        if (!ok) { std::cerr << message << '\n'; ++failures; }
    };
    send(SDL_EVENT_KEY_DOWN);
    send(SDL_EVENT_KEY_UP);
    check(input->wasActionPressed(InputAction::Jump) && input->wasActionReleased(InputAction::Jump) &&
        !input->isActionDown(InputAction::Jump), "A short tap within one frame lost an edge");
    input->update(0);
    check(!input->wasActionPressed(InputAction::Jump) && !input->wasActionReleased(InputAction::Jump),
        "Input edges survived the next frame");
    send(SDL_EVENT_KEY_DOWN);
    input->update(0);
    send(SDL_EVENT_KEY_DOWN, true);
    check(input->isActionDown(InputAction::Jump) && !input->wasActionPressed(InputAction::Jump),
        "Key repeat triggered another press");
    send(SDL_EVENT_KEY_UP);
    send(SDL_EVENT_KEY_DOWN);
    check(input->wasActionPressed(InputAction::Jump) && input->wasActionReleased(InputAction::Jump) &&
        input->isActionDown(InputAction::Jump), "Release then press within one frame lost an edge");
    send(SDL_EVENT_WINDOW_FOCUS_LOST);
    check(!input->isActionDown(InputAction::Jump) && !input->wasActionPressed(InputAction::Jump),
        "Focus loss left a stuck key");
    return failures ? 1 : 0;
}
