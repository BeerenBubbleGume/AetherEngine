//
// Created by drhaz on 14.06.2026.
//

#ifndef SMB_ENGINE_HPP
#define SMB_ENGINE_HPP

#include <chrono>
#include <expected>
#include <filesystem>
#include <iostream>
#include <ostream>
#include <ratio>
#include "assets/AssetManager.hpp"
#include "systems/InputSystem.hpp"
#include "systems/RenderSystem.hpp"
#include "resources/ResourceManager.hpp"
#include "window/Window.hpp"
#include "EngineContext.hpp"
#include "IApplication.hpp"
#include "systems/PhysicsSystem.hpp"
#include "systems/SceneSerializer.hpp"

namespace AetherEngine {
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

        auto processEvents(core::IApplication& app) -> void;
        auto update(float delta) const -> void;


        using WindowPtr = std::unique_ptr<Window, Window::WindowDeleter>;
        using InputSystemPtr = std::unique_ptr<systems::InputSystem, systems::InputSystem::InputSystemDeleter>;
        using RenderSystemPtr = std::unique_ptr<systems::RenderSystem, systems::RenderSystem::RenderSystemDeleter>;
        using ResourceManagerPtr = std::unique_ptr<resources::ResourceManager, resources::ResourceManager::ResourceManagerDeleter>;
        using AssetManagerPtr = std::unique_ptr<assets::AssetManager>;
        using ScenePtr = std::unique_ptr<scene::Scene, scene::Scene::SceneDeleter>;
        using SceneSerializerPtr = std::unique_ptr<systems::SceneSerializer, systems::SceneSerializer::SceneSerializerDeleter>;
        using PhysicsSystemPtr = std::unique_ptr<systems::PhysicsSystem, systems::PhysicsSystem::PhysicsSystemDeleter>;
        using Clock = std::chrono::steady_clock;
        using Duration = std::chrono::duration<float>;
        using TimePoint = Clock::time_point;
        WindowPtr sWindow;
        InputSystemPtr sInput;
        RenderSystemPtr sRender;
        ResourceManagerPtr sResource;
        AssetManagerPtr sAssets;
        ScenePtr sScene;
        SceneSerializerPtr sSceneSerializer;
        PhysicsSystemPtr sPhysics;

        std::filesystem::path m_assetsPath;
        float accumulator = 0.0;
        const float FIXED_DT = 1.0 / 120.0;
    };
}



#endif //SMB_ENGINE_HPP
