//
// Created by drhaz on 19.06.2026.
//

#ifndef SMB_SRENDERSYSTEM_HPP
#define SMB_SRENDERSYSTEM_HPP

#include <expected>
#include <memory>
#include <string>
#include <bgfx/bgfx.h>
#include <bgfx/platform.h>
#include <SDL3/SDL_video.h>

#include "graphics/GRenderTarget.hpp"
#include "graphics/GSceneView.hpp"
#include "resources/RResourceManager.hpp"
#include "scene/SCScene.hpp"

namespace engine::systems {
    struct SRenderSystemError {
        int code;
        std::string message;
    };
    class SYRenderSystem final {
    public:
        SYRenderSystem(const SYRenderSystem&) = delete;
        SYRenderSystem(SYRenderSystem&&) = delete;
        SYRenderSystem& operator=(const SYRenderSystem&) = delete;
        SYRenderSystem& operator=(SYRenderSystem&&) = delete;
        struct SRenderSystemDeleter {
            void operator()(SYRenderSystem* renderSystem) const {
                delete renderSystem;
            }
        };
        using SRenderSystemPtr = std::unique_ptr<SYRenderSystem, SRenderSystemDeleter>;
        static SRenderSystemPtr createRenderSystem();
        auto beginFrame() -> void;
        auto renderScene(
            scene::SCScene& scene,
            const resources::RResourceManager& resourceManager,
            const graphics::SceneView& view
        ) -> void;
        auto endFrame() -> void;

        [[nodiscard]] auto backbufferExtent() const -> graphics::RenderExtent;

        [[nodiscard]] auto init(SDL_Window& window) -> std::expected<void, SRenderSystemError>;
    private:
        SYRenderSystem() = default;
        ~SYRenderSystem();

        SDL_Window* rWindow{};
        bgfx::UniformHandle m_colorUniform = BGFX_INVALID_HANDLE;
        bgfx::UniformHandle m_samplerUniform = BGFX_INVALID_HANDLE;
        int mCurrentWidth{};
        int mCurrentHeight{};
    };
}


#endif //SMB_SRENDERSYSTEM_HPP
