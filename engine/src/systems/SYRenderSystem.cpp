//
// Created by drhaz on 19.06.2026.
//

#include "systems/SYRenderSystem.hpp"

#include <algorithm>

#include "components/IMaterialComponent.hpp"
#include "components/IMeshComponent.hpp"
#include "components/ITransformComponent.hpp"
#include "graphics/GMesh.hpp"

namespace engine::systems {
    static uint64_t toBgfxState(const resources::RRenderState& renderState) {
        uint64_t state = 0;

        if (renderState.writeRgb) {
            state |= BGFX_STATE_WRITE_RGB;
        }
        if (renderState.writeAlpha) {
            state |= BGFX_STATE_WRITE_A;
        }
        if (renderState.writeDepth) {
            state |= BGFX_STATE_WRITE_Z;
        }
        if (renderState.depthTest) {
            state |= BGFX_STATE_DEPTH_TEST_LESS;
        }
        if (renderState.cullBackFaces) {
            state |= BGFX_STATE_CULL_CW;
        }
        if (renderState.alphaBlend) {
            state |= BGFX_STATE_BLEND_ALPHA;
        }
        if (renderState.msaa) {
            state |= BGFX_STATE_MSAA;
        }

        return state;
    }

    SYRenderSystem::SRenderSystemPtr SYRenderSystem::createRenderSystem() {
        return SRenderSystemPtr(new SYRenderSystem(), SRenderSystemDeleter{});
    }

    SYRenderSystem::~SYRenderSystem() {
        if (bgfx::isValid(m_colorUniform)) {
            bgfx::destroy(m_colorUniform);
        }
        if (bgfx::isValid(m_samplerUniform)) {
            bgfx::destroy(m_samplerUniform);
        }
    }

    auto SYRenderSystem::beginFrame() -> void {
        int width = 0;
        int height = 0;
        if (!SDL_GetWindowSizeInPixels(rWindow, &width, &height)) {
            return;
        }

        width = std::max(width, 1);
        height = std::max(height, 1);
        if (mCurrentHeight == height && mCurrentWidth == width) {
            return;
        }

        mCurrentHeight = height;
        mCurrentWidth = width;
        bgfx::reset(
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height),
            BGFX_RESET_VSYNC
        );
    }

    auto SYRenderSystem::renderScene(
        scene::SCScene& scene,
        const resources::RResourceManager& resourcesManager,
        const graphics::SceneView& view
    ) -> void {
        if (view.target && !view.target->isValid()) {
            return;
        }

        bgfx::FrameBufferHandle framebuffer = BGFX_INVALID_HANDLE;
        if (view.target) {
            framebuffer = view.target->framebuffer();
        }
        const auto viewportWidth = std::max<uint16_t>(view.viewport.width, 1);
        const auto viewportHeight = std::max<uint16_t>(view.viewport.height, 1);

        bgfx::setViewFrameBuffer(view.viewId, framebuffer);
        bgfx::setViewRect(
            view.viewId,
            view.viewport.x,
            view.viewport.y,
            viewportWidth,
            viewportHeight
        );
        bgfx::setViewClear(
            view.viewId,
            view.clearFlags,
            view.clearColor,
            view.clearDepth,
            view.clearStencil
        );
        bgfx::setViewTransform(
            view.viewId,
            view.viewMatrix.data(),
            view.projectionMatrix.data()
        );
        bgfx::setViewMode(view.viewId, bgfx::ViewMode::Default);
        bgfx::touch(view.viewId);

        auto renderables = scene.view<engine::components::ITransformComponent, engine::components::IMeshComponent, engine::components::IMaterialComponent>();
        for (const auto entity : renderables) {
            auto& transform = renderables.get<engine::components::ITransformComponent>(entity);
            auto& meshComponent = renderables.get<engine::components::IMeshComponent>(entity);
            auto& materialComponent = renderables.get<engine::components::IMaterialComponent>(entity);
            const auto* mesh = resourcesManager.getMesh(meshComponent.mesh);
            const auto* program = resourcesManager.getProgram(materialComponent.material.program);
            const auto& baseColor = materialComponent.material.baseColor;
            const auto& renderState = materialComponent.material.renderState;
            if (!mesh || !program || materialComponent.material.textures.empty()) {
                continue;
            }

            const auto* texture = resourcesManager.getTexture(materialComponent.material.textures.front());
            if (!texture || !texture->isValid()) {
                continue;
            }

            if (mesh->isValid()) {
                mesh->submit(
                    program->handle(),
                    m_colorUniform,
                    m_samplerUniform,
                    texture->handle(),
                    transform.transform,
                    baseColor,
                    view.viewId,
                    toBgfxState(renderState)
                );
            }
        }
    }

    auto SYRenderSystem::endFrame() -> void {
        bgfx::frame();
    }

    auto SYRenderSystem::backbufferExtent() const -> graphics::RenderExtent {
        return {
            .width = static_cast<uint16_t>(std::max(mCurrentWidth, 1)),
            .height = static_cast<uint16_t>(std::max(mCurrentHeight, 1))
        };
    }

    auto SYRenderSystem::init(SDL_Window &window) -> std::expected<void, SRenderSystemError> {
        try {
            rWindow = &window;
            SDL_PropertiesID props = SDL_GetWindowProperties(rWindow);
            bgfx::PlatformData pd{};
            bgfx::renderFrame();
#if defined(SDL_PLATFORM_WIN32)
            pd.nwh = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);
            pd.ndt = nullptr;

#elif defined(SDL_PLATFORM_MACOS)

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
            bgfx::Init init{};

            bgfx::setPlatformData(pd);
            init.platformData = pd;
            init.type = bgfx::RendererType::Count; // авто-выбор

            int width = 0;
            int height = 0;
            if (!SDL_GetWindowSizeInPixels(rWindow, &width, &height)) {
                return std::unexpected{SRenderSystemError{1, "Failed to get window size in pixels"}};
            }
            width = std::max(width, 1);
            height = std::max(height, 1);
            init.resolution.width  = static_cast<uint32_t>(width);
            init.resolution.height = static_cast<uint32_t>(height);
            init.resolution.reset  = BGFX_RESET_VSYNC;

            if (!bgfx::init(init)) {
                return std::unexpected{SRenderSystemError{1, "Failed to initialize bgfx"}};
            }

            mCurrentWidth = width;
            mCurrentHeight = height;
            m_colorUniform = bgfx::createUniform("u_baseColor", bgfx::UniformType::Vec4);
            m_samplerUniform = bgfx::createUniform("s_albedo", bgfx::UniformType::Sampler);
            if (!bgfx::isValid(m_colorUniform) || !bgfx::isValid(m_samplerUniform)) {
                return std::unexpected{SRenderSystemError{1, "Failed to create renderer uniforms"}};
            }
            return {};
        } catch (const std::exception& e) {
            return std::unexpected{SRenderSystemError{1, e.what()}};
        }
    }
}
