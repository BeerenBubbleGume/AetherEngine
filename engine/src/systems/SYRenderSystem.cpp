//
// Created by drhaz on 19.06.2026.
//

#include "systems/SYRenderSystem.hpp"

namespace engine::systems {
    SYRenderSystem::SRenderSystemPtr SYRenderSystem::createRenderSystem() {
        return SRenderSystemPtr(new SYRenderSystem(), SRenderSystemDeleter{});
    }

    void SYRenderSystem::render(scene::SCScene &scene, const resources::RResourceManager& resourcesManager) {
        int width, height;
        SDL_GetWindowSize(rWindow, &width, &height);
        if (mCurrentHeight != height || mCurrentWidth != width) {
            mCurrentHeight = height;
            mCurrentWidth = width;
            bgfx::reset(width, height, BGFX_RESET_VSYNC);
        }
        bgfx::setViewRect(0, 0, 0, uint16_t(width), uint16_t(height));
        bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH | BGFX_CLEAR_STENCIL, 0x303030ff, 1.0f, 0);

        auto& camera = scene.getCamera();
        auto view = camera.getViewMatrix();
        auto proj = camera.getProjectionMatrix({width, height});
        bgfx::setViewTransform(0, view.data(), proj.data());
        bgfx::setViewMode(0, bgfx::ViewMode::Count);

        bgfx::touch(0);
        for (const auto& resource : scene.renderObjects()) {
            const auto& mesh = resourcesManager.getMesh(resource.mesh);
            const auto& program = resourcesManager.getProgram(resource.program);
            if (!mesh || !program) {
                continue;
            }
            if (mesh->isValid()) {
                mesh->submit(program->handle(), resource.transform, 0);
            }
        }

        bgfx::frame();
    }

    auto SYRenderSystem::init(SDL_Window &window) -> std::expected<void, SRenderSystemError> {
        try {
            rWindow = &window;
            SDL_PropertiesID props = SDL_GetWindowProperties(rWindow);
            bgfx::PlatformData pd;
#if defined(SDL_PLATFORM_WIN32)
            pd.nwh = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);
            pd.ndt = nullptr;

#elif defined(SDL_PLATFORM_MACOS)
            bgfx::renderFrame();
            pd.nwh = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_COCOA_WINDOW_POINTER, nullptr);
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
}
