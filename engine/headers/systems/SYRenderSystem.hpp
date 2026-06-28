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
#include <bx/math.h>
#include <SDL3/SDL_video.h>
#include <fstream>
#include "graphics/GMesh.hpp"

#include "graphics/GProgram.hpp"

#include "resources/RResourceManager.hpp"
#include "scene/SCScene.hpp"
#include "components/ICameraComponent.hpp"
#include "components/IMeshComponent.hpp"
#include "components/ITransformComponent.hpp"
#include "math/CameraMatricies.hpp"

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
        void render(scene::SCScene &scene, const resources::RResourceManager& recourceManager);

        [[nodiscard]] auto init(SDL_Window& window) -> std::expected<void, SRenderSystemError>;
    private:
        SYRenderSystem() = default;
        ~SYRenderSystem() = default;

        SDL_Window*                             rWindow{};
        int mCurrentWidth {};
        int mCurrentHeight {};
    };
}


#endif //SMB_SRENDERSYSTEM_HPP
