//
// Created by drhaz on 19.06.2026.
//

#include "systems/SRenderSystem.hpp"
#include "resources/RResourceManager.hpp"

namespace engine::systems {
    SRenderSystem::SRenderSystemPtr SRenderSystem::createRenderSystem() {
        return SRenderSystemPtr(new SRenderSystem(), SRenderSystemDeleter{});
    }

    void SRenderSystem::render(const resources::RResourceManager& resourceManager) const {
        int width, height;
        SDL_GetWindowSize(rWindow, &width, &height);

        bgfx::setViewRect(0, 0, 0, uint16_t(width), uint16_t(height));
        bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x303030ff, 1.0f, 0);

        // === View + Projection ===
        float view[16];
        float proj[16];

        const bgfx::Caps* caps = bgfx::getCaps();

        bx::mtxLookAt(view,
            { 0.0f, 0.0f, 5.0f },   // eye
            { 0.0f, 0.0f, 0.0f },   // at
            { 0.0f, 1.0f, 0.0f }    // up
        );

        bx::mtxProj(proj,
            60.0f,                                    // FOV
            float(width) / float(height),             // aspect
            0.1f, 100.0f,                             // near, far
            caps->homogeneousDepth                    
        );

        bgfx::setViewTransform(0, view, proj);
        bgfx::setViewMode(0, bgfx::ViewMode::Default);

        auto program = resourceManager.getProgram(m_program);
        if (!program) return;

        for (const auto& meshHandle : m_meshes) {
            auto mesh = resourceManager.getMesh(meshHandle);
            if (mesh && mesh->isValid()) {
                mesh->submit(program->handle(), m_testTransform, 0);
            }
        }

        bgfx::frame();
    }

    auto SRenderSystem::init(SDL_Window &window) -> std::expected<void, SRenderSystemError> {
        try {
            rWindow = &window;
            SDL_PropertiesID props = SDL_GetWindowProperties(rWindow);
            bgfx::PlatformData pd;
#if defined(SDL_PLATFORM_WIN32)
            pd.nwh = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);
            pd.ndt = nullptr;

#elif defined(SDL_PLATFORM_MACOS)
            pd.nwh = SDL_GetProperty(props, SDL_PROP_WINDOW_COCOA_WINDOW_POINTER, nullptr);
            pd.ndt = nullptr;

#elif defined(SDL_PLATFORM_LINUX)
            const char* driver = SDL_GetCurrentVideoDriver();
            if (SDL_strcmp(driver, "x11") == 0) {
                pd.ndt = SDL_GetProperty(props, SDL_PROP_WINDOW_X11_DISPLAY_POINTER, nullptr);
                pd.nwh = (void*)(uintptr_t)SDL_GetNumberProperty(props, SDL_PROP_WINDOW_X11_WINDOW_NUMBER, 0);
            } else if (SDL_strcmp(driver, "wayland") == 0) {
                pd.ndt = SDL_GetProperty(props, SDL_PROP_WINDOW_WAYLAND_DISPLAY_POINTER, nullptr);
                pd.nwh = SDL_GetProperty(props, SDL_PROP_WINDOW_WAYLAND_SURFACE_POINTER, nullptr);
            }
#endif
            bgfx::Init init;

            bgfx::setPlatformData(pd);
            init.platformData = pd;
            init.type = bgfx::RendererType::Count; // авто-выбор

            int width, height;
            SDL_GetWindowSize(rWindow, &width, &height);
            init.resolution.width  = static_cast<uint32_t>(width);
            init.resolution.height = static_cast<uint32_t>(height);
            init.resolution.reset  = BGFX_RESET_VSYNC;

            if (!bgfx::init(init)) {
                return std::unexpected{SRenderSystemError{1, "Failed to initialize bgfx"}};
            }

            bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x303030ff, 1.0f, 0);
            bgfx::setViewRect(0, 0, 0, uint16_t(width), uint16_t(height));

            return {};
        } catch (const std::exception& e) {
            return std::unexpected{SRenderSystemError{1, e.what()}};
        }
    }

    auto SRenderSystem::addMesh(resources::RMeshHandle mesh) -> std::expected<void, SRenderSystemError> {
        if (!mesh.isValid()) {
            return std::unexpected{SRenderSystemError{1, "Mesh handle is invalid"}};
        }
        m_meshes.push_back(mesh);
        return {};
    }

    auto SRenderSystem::getTransform() -> graphics::GTransform & {
        return m_testTransform;
    }

    auto SRenderSystem::setProgram(resources::RProgramHandle program) -> std::expected<void, SRenderSystemError> {
        if (!program.isValid()) {
            return std::unexpected{SRenderSystemError{1, "Program handle is invalid"}};
        }
        m_program = program;
        return {};
    }
}
