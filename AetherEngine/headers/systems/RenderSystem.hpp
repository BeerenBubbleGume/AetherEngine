//
// Created by drhaz on 19.06.2026.
//

#ifndef SMB_RENDERSYSTEM_HPP
#define SMB_RENDERSYSTEM_HPP

#include <expected>
#include <memory>
#include <string>
#include <bgfx/bgfx.h>
#include <bgfx/platform.h>
#include <SDL3/SDL_video.h>

#include "SceneAssetBindingSystem.hpp"
#include "assets/AssetManager.hpp"
#include "graphics/RenderTarget.hpp"
#include "graphics/SceneView.hpp"
#include "scene/Scene.hpp"

namespace AetherEngine::systems {
    struct RenderSystemError {
        int code;
        std::string message;
    };
    class RenderSystem final {
    public:
        RenderSystem(const RenderSystem&) = delete;
        RenderSystem(RenderSystem&&) = delete;
        RenderSystem& operator=(const RenderSystem&) = delete;
        RenderSystem& operator=(RenderSystem&&) = delete;
        struct RenderSystemDeleter {
            void operator()(RenderSystem* renderSystem) const {
                delete renderSystem;
            }
        };
        using RenderSystemPtr = std::unique_ptr<RenderSystem, RenderSystemDeleter>;
        static RenderSystemPtr createRenderSystem();
        auto beginFrame() -> void;
        auto renderScene(
            scene::Scene& scene,
            const assets::AssetManager& assetManager,
            const SceneAssetBindingSystem& sceneAssets, const graphics::SceneView& view
        ) const -> void;

        static auto endFrame() -> void;

        [[nodiscard]] auto backbufferExtent() const -> graphics::RenderExtent;

        [[nodiscard]] auto init(SDL_Window& window) -> std::expected<void, RenderSystemError>;
    private:
        RenderSystem() = default;
        ~RenderSystem();

        SDL_Window* rWindow{};
        bgfx::UniformHandle m_colorUniform = BGFX_INVALID_HANDLE;
        bgfx::UniformHandle m_samplerUniform = BGFX_INVALID_HANDLE;
        int mCurrentWidth{};
        int mCurrentHeight{};
        bool m_initialized{false};
    };
}


#endif //SMB_RENDERSYSTEM_HPP
