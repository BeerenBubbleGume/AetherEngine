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

namespace engine::systems {
    struct SRenderSystemError {
        int code;
        std::string message;
    };
    class SRenderSystem final {
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
        void render() const;

        [[nodiscard]] auto init(SDL_Window& window) -> std::expected<void, SRenderSystemError>;
        [[nodiscard]] auto setProgram(graphics::GProgram::GProgramPtr program) -> std::expected<void, SRenderSystemError>;
        [[nodiscard]] auto addMesh(graphics::GMesh::GMeshPtr mesh) -> std::expected<void, SRenderSystemError>;
        [[nodiscard]] auto getTransform() -> graphics::GTransform &;
    private:
        SRenderSystem() = default;
        ~SRenderSystem() = default;

        SDL_Window*                             rWindow{};
        graphics::GProgram::GProgramPtr         m_program;

        std::vector<graphics::GMesh::GMeshPtr>  m_meshes;
        graphics::GTransform                    m_testTransform;
    };
}


#endif //SMB_SRENDERSYSTEM_HPP
