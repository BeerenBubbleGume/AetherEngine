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

namespace engine::systems {
    struct SRenderSystemError {
        int code;
        std::string message;
    };
    class SRenderSystem {
    public:
        SRenderSystem(const SRenderSystem&) = delete;
        SRenderSystem(SRenderSystem&&) = delete;
        SRenderSystem& operator=(const SRenderSystem&) = delete;
        SRenderSystem& operator=(SRenderSystem&&) = delete;
        struct SRenderSystemDeleter {
            void operator()(SRenderSystem* renderSystem) const {
                delete renderSystem;
            }
        };
        using SRenderSystemPtr = std::unique_ptr<SRenderSystem, SRenderSystemDeleter>;
        static SRenderSystemPtr createRenderSystem();
        void render();

        [[nodiscard]] auto init(SDL_Window& window) -> std::expected<void, SRenderSystemError>;
    private:
        SRenderSystem() = default;
        ~SRenderSystem();

        SDL_Window* rWindow;
    };
}


#endif //SMB_SRENDERSYSTEM_HPP
