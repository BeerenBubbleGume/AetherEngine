//
// Created by drhaz on 22.06.2026.
//

#ifndef SMB_ENGINECONTEXT_HPP
#define SMB_ENGINECONTEXT_HPP

#include <filesystem>
#include <functional>

#include "scene/SCScene.hpp"
#include "systems/SYInputSystem.hpp"
#include "systems/SySceneSerializerSystem.hpp"
#include "window/Window.hpp"

namespace engine::systems { class SYRenderSystem; }

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
        scene::SCScene& scene;
        resources::RResourceManager& resources;
        systems::SYInputSystem& input;
        systems::SYRenderSystem& renderer;
        systems::SYSceneSerializerSystem& sceneSerializer;

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
