//
// Created by drhaz on 14.06.2026.
//

#include "core/Engine.hpp"

#include <algorithm>
#include <cmath>

#include "security/PathSecurity.hpp"

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#endif

auto engine::Engine::initEngine() -> std::expected<void, core::EngineError> {
#if defined(_WIN32)
    constexpr DWORD SafeDllDirectories =
        LOAD_LIBRARY_SEARCH_APPLICATION_DIR |
        LOAD_LIBRARY_SEARCH_SYSTEM32;
    if (!SetDefaultDllDirectories(SafeDllDirectories) || !SetDllDirectoryW(L"")) {
        return std::unexpected{core::EngineError{1, "Failed to secure the Windows DLL search path"}};
    }
#endif

    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return std::unexpected{core::EngineError{1, "Failed to initialize SDL"}};
    }
    const char* executableBasePath = SDL_GetBasePath();
    if (executableBasePath == nullptr || *executableBasePath == '\0') {
        return std::unexpected{core::EngineError{2, "Failed to determine the executable directory"}};
    }
    auto canonicalAssetsPath = security::canonicalDirectory(
        std::filesystem::path{executableBasePath} / "assets"
    );
    if (!canonicalAssetsPath) {
        return std::unexpected{core::EngineError{2, canonicalAssetsPath.error().message}};
    }
    m_assetsPath = std::move(*canonicalAssetsPath);

    sWindow = Window::createWindow();
    sInput = systems::InputSystem::createInputSystem();
    sRender = systems::RenderSystem::createRenderSystem();
    sResource = resources::ResourceManager::createResourceManager(m_assetsPath);
    sSceneSerializer = systems::SceneSerializer::createSceneSerializer(m_assetsPath / "scenes");
    sPhysics = systems::PhysicsSystem::createPhysicsSystem();
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
    if (!sSceneSerializer) {
        return std::unexpected{core::EngineError{2, "Failed to create scene serializer system"}};
    }
    if (!sPhysics) {
        return std::unexpected{core::EngineError{2, "Failed to create physics system"}};
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

    auto physicsInitResult = sPhysics->init();
    if (!physicsInitResult) {
        std::cerr << "Failed to init physics: " << physicsInitResult.error().message << std::endl;
        return std::unexpected{core::EngineError{3, "Failed to init physics"}};
    }
    try {
        auto loadedScene = sSceneSerializer->deserializeScene(
        "DefaultScene",
        {
            .resources = sResource.get(),
            .assetsRoot = m_assetsPath
        }
    );

        if (loadedScene) {
            sScene = std::move(loadedScene.value());
        } else {
            sScene = scene::Scene::createScene("DefaultScene");
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
    constexpr float MaxFrameDelta = 0.25f;
    constexpr std::size_t MaxPhysicsSubsteps = 8;
    core::EngineContext ctx{
        .scene = *sScene,
        .resources = *sResource,
        .input = *sInput,
        .renderer = *sRender,
        .sceneSerializer = *sSceneSerializer,
        .window = *sWindow,
        .paths = {m_assetsPath, m_assetsPath / "shaders"},
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
        if (!std::isfinite(delta)) {
            delta = 0.0f;
        }
        delta = std::clamp(delta, 0.0f, MaxFrameDelta);
        auto runConfig = app.runConfig();

        accumulator = std::min(accumulator + delta, FIXED_DT * MaxPhysicsSubsteps);
        update(delta);
        processEvents(app);
        if (runConfig.updatePhysics) {
            std::size_t substeps = 0;
            while (accumulator >= FIXED_DT && substeps < MaxPhysicsSubsteps) {
                sPhysics->fixedUpdate(*sScene, FIXED_DT);
                accumulator -= FIXED_DT;
                ++substeps;
            }
            if (substeps == MaxPhysicsSubsteps) {
                accumulator = 0.0f;
            }
        } else {
            accumulator = 0.f;
        }

        app.update(delta, ctx);
        sRender->beginFrame();
        app.render(ctx);
        sRender->endFrame();
    }
    app.shutdown(ctx);
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
    if (sWindow) {
        sWindow.reset();
    }
    SDL_Quit();
}

auto engine::Engine::processEvents(core::IApplication& app) -> void {
    SDL_Event event;

    while (SDL_PollEvent(&event))
    {
        if (event.type == SDL_EVENT_QUIT){
            isRunning = false;
        }

        app.onEvent(event);
        sInput->processEvents(event);
    }
}

auto engine::Engine::update(float delta) const -> void {
    sInput->update(delta);
}
