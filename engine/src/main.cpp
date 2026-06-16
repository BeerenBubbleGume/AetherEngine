#include <SDL3/SDL.h>
#include <entt/entt.hpp>
#include "window/Window.hpp"

int main()
{
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return 1;
    }

    auto window = engine::Window::createWindow();

    if (!window->initWindow("SMB Engine", 800, 600))
    {
        SDL_Quit();
        return 1;
    }

    bool running = true;
    SDL_Event event;

    while (running)
    {
        while (SDL_PollEvent(&event))
        {
            SDL_SetWindowTitle(window->getSDLWindow(), "alive");
            if (event.type == SDL_EVENT_QUIT)
            {
                running = false;
            }
        }

        // пока ничего не рендерим — просто “жизнь окна”
        SDL_Delay(1);
    }

    SDL_Quit();
    return 0;
}