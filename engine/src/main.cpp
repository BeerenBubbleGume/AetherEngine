#include <SDL3/SDL.h>
#include <entt/entt.hpp>
#include "Engine.hpp"

int main()
{
    auto engine = engine::Engine::createEngine();
    if (!engine) {
        SDL_Log("Failed to create engine");
        return 1;
    }
    auto initResult = engine->initEngine();
    if (!initResult) {
        SDL_Log("Failed to init engine");
        return 1;
    }
    auto result = engine->run();
    if (!result) {
        SDL_Log("Failed to run engine");
        return 1;
    }
    return 0;
}