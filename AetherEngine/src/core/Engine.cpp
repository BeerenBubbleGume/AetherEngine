//
// Created by drhaz on 14.06.2026.
//

#include "core/AetherEngine.hpp"

#include <algorithm>
#include <cmath>
#include <spdlog/sinks/stdout_sinks.h>

#include "logging/SpdlogSupport.hpp"

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#endif

namespace {
    class StderrLogSink final : public AetherEngine::logging::ILogSink {
    public:
        StderrLogSink() { m_sink.set_pattern("[%l][%n] %v"); }

        auto write(const AetherEngine::logging::LogRecord& record) -> void override {
            using namespace AetherEngine::logging::detail;
            const spdlog::details::log_msg message{
                record.timestamp, spdlog::source_loc{}, toSpdlogView(record.category),
                toSpdlogLevel(record.level), toSpdlogView(record.message)};
            m_sink.log(message);
        }

    private:
        spdlog::sinks::stderr_sink_mt m_sink;
    };
}

AetherEngine::Engine::Engine() {
    m_logger.addSink(std::make_shared<StderrLogSink>());
}

auto AetherEngine::Engine::initEngine(const core::EngineInitConfig& config) -> std::expected<void, core::EngineError> {
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
    auto paths = core::resolveEnginePaths(config, std::filesystem::path{executableBasePath});
    if (!paths) {
        return std::unexpected(paths.error());
    }
    m_paths = std::move(*paths);

    sWindow = Window::createWindow();
    sInput = systems::InputSystem::createInputSystem();
    sRender = systems::RenderSystem::createRenderSystem();
    sResource = resources::ResourceManager::createResourceManager(m_paths.assetsRoot);
    sSceneSerializer = systems::SceneSerializer::createSceneSerializer(m_paths.scenesRoot);
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
    // Opening an existing project must never silently replace a broken scene
    // with an empty document (which the application could then save over it).
    auto loadedScene = sSceneSerializer->deserializeScene(config.startupScene);
    if (!loadedScene) {
        return std::unexpected(core::EngineError{
            3, "Failed to load scene '" + config.startupScene + "': " + loadedScene.error().message
        });
    }
    sScene = std::move(*loadedScene);
    auto resultInitWindow = sWindow->initWindow("SMB Engine", 1440, 1080);
    if (!resultInitWindow) {
        m_logger.error("Failed to init window: " + resultInitWindow.error().message, "Engine");
        return std::unexpected{core::EngineError{3, "Failed to init window"}};
    }
    auto resultInitRenderer = sRender->init(*sWindow->getSDLWindow());
    if (!resultInitRenderer) {
        m_logger.error("Failed to init renderer: " + resultInitRenderer.error().message, "Engine");
        return std::unexpected{core::EngineError{3, "Failed to init renderer"}};
    }

    const auto platform = assets::detectPlatformProfile(bgfx::getRendererType());
    if (!platform) {
        return std::unexpected{core::EngineError{3, "The active renderer has no supported asset platform profile"}};
    }
    auto assetManager = assets::AssetManager::create(m_paths.assetsRoot, *sResource, *platform);
    if (!assetManager) {
        return std::unexpected{core::EngineError{
            3,
            "Failed to initialize asset manager: " + assetManager.error().message
        }};
    }
    sAssets = std::move(*assetManager);
    sSceneAssetBinding = systems::SceneAssetBindingSystem::create(*sAssets);
    auto physicsInitResult = sPhysics->init();
    if (!physicsInitResult) {
        m_logger.error("Failed to init physics: " + physicsInitResult.error().message, "Engine");
        return std::unexpected{core::EngineError{3, "Failed to init physics"}};
    }
    state = core::EngineState::Initialized;
    return std::expected<void, core::EngineError>{};
}

auto AetherEngine::Engine::run(core::IApplication &app) -> std::expected<void, core::EngineError> {
    state = core::EngineState::Running;
    TimePoint lastTime = Clock::now();
    constexpr float MaxFrameDelta = 0.25f;
    constexpr std::size_t MaxPhysicsSubsteps = 8;
    core::EngineContext ctx{
        .logger = m_logger,
        .assets = *sAssets,
        .input = *sInput,
        .renderer = *sRender,
        .sceneSerializer = *sSceneSerializer,
        .sceneAssetBinding = *sSceneAssetBinding,
        .window = *sWindow,
        .paths = m_paths,
        .requestQuit = [this]() { state = core::EngineState::Stopped; },
        .getScene = [this]() -> scene::Scene& { return *sScene; },
        .replaceScene = [this](scene::Scene::ScenePtr next) -> std::expected<void, core::EngineError> {
            return replaceScene(std::move(next));
        }
    };
    auto appInitResult = app.init(ctx);
    if (!appInitResult) {
        m_logger.error("Failed to init application: " + appInitResult.error().message, "Engine");
        return std::unexpected{core::EngineError{4, "Failed to init application"}};
    }
    auto sceneBindResult = sSceneAssetBinding->bindScene(*sScene);
    if (!sceneBindResult) {
        m_logger.error("Failed to bind scene: " + sceneBindResult.error().message, "Engine");
        return std::unexpected{core::EngineError{4, "Failed to bind scene"}};
    }
    try {
        while (state == core::EngineState::Running) {
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
            auto sceneSynchronizeResult = sSceneAssetBinding->synchronize(*sScene);
            if (!sceneSynchronizeResult) {
                m_logger.error("Failed to synchronize scene: " + sceneSynchronizeResult.error().message, "Engine");
                state = core::EngineState::Stopped;
                throw std::runtime_error{sceneSynchronizeResult.error().message};
            }
            sRender->beginFrame();
            app.render(ctx);
            sRender->endFrame();
        }
    } catch (const std::exception& e) {
        app.shutdown(ctx);
        m_logger.error(std::string{"Exception during engine run: "} + e.what(), "Engine");
        return std::unexpected{core::EngineError{5, "Exception during engine run"}};
    } catch (...) {
        app.shutdown(ctx);
        m_logger.error("Unknown exception during engine run", "Engine");
        return std::unexpected{core::EngineError{5, "Unknown exception during engine run"}};
    }

    app.shutdown(ctx);
    return std::expected<void, core::EngineError>{};
}

AetherEngine::Engine::EnginePtr AetherEngine::Engine::createEngine() {
    return EnginePtr(new Engine, EngineDeleter{});
}

AetherEngine::Engine::~Engine() {
    if (sSceneAssetBinding) {
        sSceneAssetBinding.reset();
    }
    if (sScene) {
        sScene.reset();
    }
    if (sAssets) {
        sAssets.reset();
    }
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

auto AetherEngine::Engine::processEvents(core::IApplication& app) -> void {
    SDL_Event event;

    while (SDL_PollEvent(&event))
    {
        if (event.type == SDL_EVENT_QUIT){
            state = core::EngineState::Stopped;
        }

        app.onEvent(event);
        sInput->processEvents(event);
    }
}

auto AetherEngine::Engine::update(float delta) const -> void {
    sInput->update(delta);
}

auto AetherEngine::Engine::replaceScene(scene::Scene::ScenePtr newScene) -> std::expected<void, core::EngineError> {
    try {
        if (!newScene) {
            return std::unexpected{core::EngineError{6, "Cannot replace scene with a null scene"}};
        }
        auto sceneBindResult = sSceneAssetBinding->bindScene(*newScene);
        if (!sceneBindResult) {
            return std::unexpected{core::EngineError{6, "Failed to bind new scene: " + sceneBindResult.error().message}};
        }
        sPhysics->clearSceneBodies();
        accumulator = 0.0f;
        sScene = std::move(newScene);
    } catch (...) {
        m_logger.error("Exception during engine::replaceScene", "Engine");
        return std::unexpected{core::EngineError{6, "Exception during engine::replaceScene"}};
    }
    return {};
}
