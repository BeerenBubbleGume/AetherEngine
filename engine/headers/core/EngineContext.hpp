//
// Created by drhaz on 22.06.2026.
//

#ifndef SMB_ENGINECONTEXT_HPP
#define SMB_ENGINECONTEXT_HPP

#include <filesystem>
#include <functional>

#include "scene/Scene.hpp"
#include "systems/InputSystem.hpp"
#include "systems/SceneSerializer.hpp"
#include "window/Window.hpp"

namespace engine::systems { class RenderSystem; }

namespace engine::core {
    class EngineError {
    public:
        int code;
        std::string message;
    };
    struct EnginePaths {
        std::filesystem::path assetsRoot;
        std::filesystem::path shadersRoot;
    };

    struct EngineContext {
        scene::Scene& scene;
        resources::ResourceManager& resources;
        systems::InputSystem& input;
        systems::RenderSystem& renderer;
        systems::SceneSerializer& sceneSerializer;

        const Window& window;
        const EnginePaths& paths;

        std::function<void()> requestQuit;
    };

    struct EngineRunConfig {
        bool updatePhysics{true};
    };
} // core
// engine

#endif //SMB_ENGINECONTEXT_HPP
