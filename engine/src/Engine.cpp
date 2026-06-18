//
// Created by drhaz on 14.06.2026.
//

#include "Engine.hpp"

#include <iostream>
#include <ostream>

auto engine::Engine::initEngine() -> std::expected<void, EngineError> {
    window = Window::createWindow();
    if (!window) {
        return std::unexpected{EngineError{1, "Failed to create window"}};
    }
    auto resultInitWindow = window->initWindow("SMB Engine", 800, 600);
    if (!resultInitWindow) {
        std::cerr << "Failed to init window: " << resultInitWindow.error().message << std::endl;
        return std::unexpected{EngineError{2, "Failed to init window"}};
    }
    return std::expected<void, EngineError>{};
}

auto engine::Engine::run() -> std::expected<void, EngineError> {
    isRunning = true;
    SDL_Event event;
    while (isRunning)
    {
        while (SDL_PollEvent(&event))
        {
            SDL_SetWindowTitle(window->getSDLWindow(), "alive");
            if (event.type == SDL_EVENT_QUIT)
            {
                isRunning = false;
            }
        }

        // пока ничего не рендерим — просто “жизнь окна”
        SDL_Delay(1);
    }
    return std::expected<void, EngineError>{};
}

auto engine::Engine::shutdown() const -> std::expected<void, EngineError> {
    if (window) {
        auto result = window->shutdown();
        if (!result) {
            return std::unexpected{EngineError{2, ("Failed to shutdown window: " + std::string(result.error().message)).c_str()}};
        }
    }
    return std::expected<void, EngineError>{};
}

engine::Engine::EnginePtr engine::Engine::createEngine() {
    return std::unique_ptr<Engine, EngineDeleter>(new Engine, EngineDeleter{});
}

engine::Engine::~Engine() {
    auto result = shutdown();
    if (!result) {
        std::cerr << "Failed to shutdown engine: " << result.error().message << std::endl;
    }
    if (window) {
        window.release();
    }
}
