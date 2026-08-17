//
// Created by drhaz on 22.06.2026.
//

#ifndef SMB_ENGINECONTEXT_HPP
#define SMB_ENGINECONTEXT_HPP

#include <filesystem>
#include <functional>

#include "assets/AssetManager.hpp"
#include "scene/Scene.hpp"
#include "systems/InputSystem.hpp"
#include "systems/SceneSerializer.hpp"
#include "window/Window.hpp"

namespace AetherEngine::systems { class RenderSystem; }

namespace AetherEngine::core {
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
        assets::AssetManager& assets;
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
// AetherEngine

#endif //SMB_ENGINECONTEXT_HPP
