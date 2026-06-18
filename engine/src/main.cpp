#include <SDL3/SDL.h>
#include <entt/entt.hpp>
#include "Engine.hpp"

int main()
{
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return 1;
    }

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
    SDL_Quit();
    return 0;
}