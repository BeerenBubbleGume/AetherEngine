//
// Created by drhaz on 14.06.2026.
//

#ifndef SMB_ENGINE_HPP
#define SMB_ENGINE_HPP

#include <chrono>
#include <expected>
#include <chrono>
#include <iostream>
#include <ostream>
#include <ratio>
#include "systems/SYInputSystem.hpp"
#include "systems/SYRenderSystem.hpp"
#include "resources/RResourceManager.hpp"
#include "window/Window.hpp"
#include "EngineContext.hpp"
#include "IApplication.hpp"

namespace engine {
    class Engine final {
    public:
        Engine(const Engine&) = delete;
        Engine& operator=(const Engine&) = delete;
        Engine(Engine&&) = delete;
        Engine& operator=(Engine&&) = delete;

        struct EngineDeleter {
            void operator()(const Engine* engine) const {
                delete engine;
            }
        };
        using EnginePtr = std::unique_ptr<Engine, EngineDeleter>;

        [[nodiscard]] auto initEngine() -> std::expected<void, core::EngineError>;
        [[nodiscard]] auto run(core::IApplication &app) -> std::expected<void, core::EngineError>;
        static EnginePtr createEngine();
    private:
        Engine() = default;
        ~Engine();

        bool isRunning{false};

        [[nodiscard]] auto render() -> std::expected<void, core::EngineError>;

        auto processEvents() -> void;
        auto update(float delta) const -> void;


        using WindowPtr = std::unique_ptr<Window, Window::WindowDeleter>;
        using SInputPtr = std::unique_ptr<systems::SYInputSystem, systems::SYInputSystem::SInputDeleter>;
        using SRenderPtr = std::unique_ptr<systems::SYRenderSystem, systems::SYRenderSystem::SRenderSystemDeleter>;
        using SResourcePtr = std::unique_ptr<resources::RResourceManager, resources::RResourceManager::SResourceManagerDeleter>;
        using SCScenePtr = std::unique_ptr<scene::SCScene, scene::SCScene::SCSceneDeleter>;
        using Clock = std::chrono::steady_clock;
        using Duration = std::chrono::duration<float>;
        using TimePoint = Clock::time_point;
        WindowPtr sWindow;
        SInputPtr sInput;
        SRenderPtr sRender;
        SResourcePtr sResource;
        SCScenePtr sScene;

        float accumulator = 0.0;
        const float FIXED_DT = 1.0 / 120.0;
    };
}



#endif //SMB_ENGINE_HPP
