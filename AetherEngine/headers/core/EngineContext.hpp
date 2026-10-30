//
// Created by drhaz on 22.06.2026.
//

#ifndef SMB_ENGINECONTEXT_HPP
#define SMB_ENGINECONTEXT_HPP

#include <filesystem>
#include <functional>

#include "EngineConfig.hpp"

#include "assets/AssetManager.hpp"
#include "logging/Logger.hpp"
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
        logging::Logger& logger;
        assets::AssetManager& assets;
        systems::InputSystem& input;
        systems::RenderSystem& renderer;
        systems::SceneSerializer& sceneSerializer;
        systems::SceneAssetBindingSystem& sceneAssetBinding;

        const Window& window;
        const EnginePaths& paths;

        std::function<void()> requestQuit;
        std::function<scene::Scene&()> getScene;
        std::function<std::expected<void, EngineError>(scene::Scene::ScenePtr)> replaceScene;
        // Success swaps ownership: other receives the previous active scene.
        // Failure (including exceptions) leaves both owners and the active binding intact.
        // Callbacks must perform all fallible work before committing the swap.
        std::function<std::expected<void, EngineError>(scene::Scene::ScenePtr&)> exchangeScene;
    };

    struct EngineRunConfig {
        bool updatePhysics{true};
        bool allowMissingAssets{false};
    };
} // core
// AetherEngine

#endif //SMB_ENGINECONTEXT_HPP
