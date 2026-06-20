//
// Created by drhaz on 14.06.2026.
//

#include "Engine.hpp"

auto engine::Engine::initEngine() -> std::expected<void, EngineError> {
    sWindow = Window::createWindow();
    sInput = systems::SInputSystem::createInputSystem();
    sRender = systems::SRenderSystem::createRenderSystem();
    if (!sWindow) {
        return std::unexpected{EngineError{1, "Failed to create window"}};
    }
    if (!sInput) {
        return std::unexpected{EngineError{1, "Failed to create input system"}};
    }
    if (!sRender) {
        return std::unexpected{EngineError{1, "Failed to create render system"}};
    }
    auto resultInitWindow = sWindow->initWindow("SMB Engine", 800, 600);
    if (!resultInitWindow) {
        std::cerr << "Failed to init window: " << resultInitWindow.error().message << std::endl;
        return std::unexpected{EngineError{2, "Failed to init window"}};
    }
    auto resultInitRenderer = sRender->init(*sWindow->getSDLWindow());
    if (!resultInitRenderer) {
        std::cerr << "Failed to init renderer: " << resultInitRenderer.error().message << std::endl;
        return std::unexpected{EngineError{2, "Failed to init renderer"}};
    }
    return std::expected<void, EngineError>{};
}

auto engine::Engine::run() -> std::expected<void, EngineError> {
    isRunning = true;
    TimePoint lastTime = Clock::now();
    auto mesh = graphics::GMesh::createMesh();
    if (!mesh) {
        std::cerr << "Failed to create mesh" << std::endl;
        return std::unexpected{EngineError{2, "Failed to create mesh"}};
    }
    mesh->createTriangle();
    auto resultMeshAdd = sRender->addMesh(std::move(mesh));
    if (!resultMeshAdd) {
        std::cerr << "Failed to add mesh: " << resultMeshAdd.error().message << std::endl;
        return std::unexpected{EngineError{2, "Failed to add mesh"}};
    }

    while (isRunning) {
        TimePoint now = Clock::now();
        auto delta = std::chrono::duration<float>(now - lastTime).count();
        lastTime = now;

        accumulator += delta;
        processEvents();
        while (accumulator >= FIXED_DT) {
            accumulator -= FIXED_DT;
        }
        sRender->render();
        update(delta);
    }
    return std::expected<void, EngineError>{};
}

auto engine::Engine::shutdown() const -> std::expected<void, EngineError> {
    if (sWindow) {
        auto result = sWindow->shutdown();
        if (!result) {
            return std::unexpected{EngineError{2, ("Failed to shutdown window: " + result.error().message)}};
        }
    }
    if (sRender) {
        // sRender->shutdown(); // Assuming shutdown is handled in the destructor
    }
    return std::expected<void, EngineError>{};
}

engine::Engine::EnginePtr engine::Engine::createEngine() {
    return EnginePtr(new Engine, EngineDeleter{});
}

engine::Engine::~Engine() {
    auto result = shutdown();
    if (!result) {
        std::cerr << "Failed to shutdown engine: " << result.error().message << std::endl;
    }
    if (sWindow) {
        sWindow.reset();
    }
    if (sInput) {
        sInput.reset();
    }
}

auto engine::Engine::processEvents() -> void {
    SDL_Event event;

    while (SDL_PollEvent(&event))
    {
        if (event.type == SDL_EVENT_QUIT){
            isRunning = false;
        }

        sInput->processEvents(event);
    }
}

auto engine::Engine::update(float delta) const -> void {
    sInput->update(delta);
}
