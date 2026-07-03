//
// Created by drhaz on 14.06.2026.
//

#include "core/Engine.hpp"

auto engine::Engine::initEngine() -> std::expected<void, core::EngineError> {
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return std::unexpected{core::EngineError{1, "Failed to initialize SDL"}};
    }
    sWindow = Window::createWindow();
    sInput = systems::SYInputSystem::createInputSystem();
    sRender = systems::SYRenderSystem::createRenderSystem();
    sResource = resources::RResourceManager::createResourceManager();
    sSceneSerializer = systems::SYSceneSerializerSystem::createSceneSerializerSystem(std::filesystem::current_path() / "assets" / "scenes");
    if (!sWindow) {
        return std::unexpected{core::EngineError{2, "Failed to create window"}};
    }
    if (!sInput) {
        return std::unexpected{core::EngineError{2, "Failed to create input system"}};
    }
    if (!sRender) {
        return std::unexpected{core::EngineError{2, "Failed to create render system"}};
    }
    if (!sResource) {
        return std::unexpected{core::EngineError{2, "Failed to create resource manager"}};
    }
    auto resultInitWindow = sWindow->initWindow("SMB Engine", 1440, 1080);
    if (!resultInitWindow) {
        std::cerr << "Failed to init window: " << resultInitWindow.error().message << std::endl;
        return std::unexpected{core::EngineError{3, "Failed to init window"}};
    }
    auto resultInitRenderer = sRender->init(*sWindow->getSDLWindow());
    if (!resultInitRenderer) {
        std::cerr << "Failed to init renderer: " << resultInitRenderer.error().message << std::endl;
        return std::unexpected{core::EngineError{3, "Failed to init renderer"}};
    }
    const auto assetsPath = std::filesystem::current_path() / "assets";

    try {
        auto loadedScene = sSceneSerializer->deserializeScene(
        "DefaultScene",
        {
            .resources = sResource.get(),
            .assetsRoot = assetsPath
        }
    );

        if (loadedScene) {
            sScene = std::move(loadedScene.value());
        } else {
            sScene = scene::SCScene::createScene("DefaultScene");
        }
    } catch (const std::exception& e) {
        std::cerr << "Failed to load scene: " << e.what() << std::endl;
        return std::unexpected{core::EngineError{3, "Failed to load scene"}};
    }

    return std::expected<void, core::EngineError>{};
}

auto engine::Engine::run(core::IApplication &app) -> std::expected<void, core::EngineError> {
    isRunning = true;
    TimePoint lastTime = Clock::now();
    const std::filesystem::path rootPath = std::filesystem::current_path();
    std::filesystem::path assetsPath = rootPath / "assets";

    core::EngineContext ctx{
        .scene = *sScene,
        .resources = *sResource,
        .input = *sInput,
        .sceneSerializer = *sSceneSerializer,
        .window = *sWindow,
        .paths = {assetsPath, assetsPath / "shaders"},
        .requestQuit = [&]() { isRunning = false; }
    };
    auto appInitResult = app.init(ctx);
    if (!appInitResult) {
        std::cerr << "Failed to init application: " << appInitResult.error().message << std::endl;
        return std::unexpected{core::EngineError{4, "Failed to init application"}};
    }

    while (isRunning) {
        TimePoint now = Clock::now();
        auto delta = std::chrono::duration<float>(now - lastTime).count();
        lastTime = now;

        accumulator += delta;
        update(delta);
        processEvents();
        while (accumulator >= FIXED_DT) {
            accumulator -= FIXED_DT;
        }
        app.update(delta, ctx);
        sRender->render(*sScene, *sResource);
    }
    return std::expected<void, core::EngineError>{};
}

engine::Engine::EnginePtr engine::Engine::createEngine() {
    return EnginePtr(new Engine, EngineDeleter{});
}

engine::Engine::~Engine() {
    if (sResource) {
        sResource.reset();
    }
    if (sInput) {
        sInput.reset();
    }
    if (sRender) {
        sRender.reset();
    }
    bgfx::shutdown();
    if (sWindow) {
        sWindow.reset();
    }
    SDL_Quit();
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
