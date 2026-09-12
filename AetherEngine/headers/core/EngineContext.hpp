//
// Created by drhaz on 22.06.2026.
//

#ifndef SMB_ENGINECONTEXT_HPP
#define SMB_ENGINECONTEXT_HPP

#include <filesystem>
#include <functional>

#include "EngineConfig.hpp"

#include "assets/AssetManager.hpp"
#include "scene/Scene.hpp"
#include "systems/InputSystem.hpp"
#include "systems/SceneSerializer.hpp"
#include "window/Window.hpp"
#include "systems/SceneAssetBindingSystem.hpp"

namespace AetherEngine::systems { class RenderSystem; }

namespace AetherEngine::core {
    enum class EngineState {
        Starting,
        Initialized,
        Running,
        Stopped
    };

    struct EngineContext {
        scene::Scene& scene;
        assets::AssetManager& assets;
        systems::InputSystem& input;
        systems::RenderSystem& renderer;
        systems::SceneSerializer& sceneSerializer;
        systems::SceneAssetBindingSystem& sceneAssetBinding;

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
