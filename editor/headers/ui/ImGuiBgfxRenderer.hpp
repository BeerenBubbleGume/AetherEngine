//
// Created by drhaz on 10.07.2026.
//

#ifndef SMB_IMGUIBGFXRENDERER_HPP
#define SMB_IMGUIBGFXRENDERER_HPP

#include <expected>
#include <imgui.h>
#include <memory>

#include <bgfx/bgfx.h>

#include "core/ETypes.hpp"
#include "graphics/GRenderTarget.hpp"


namespace AetherEditor::ui {
    struct ImGuiBgfxRendererState final {
        bgfx::Encoder* encoder{nullptr};
        bgfx::ViewId viewId{0};
        uint32_t samplerFlags{0};
    };

    class ImGuiBgfxRenderer final {
    public:
        struct ImGuiBgfxRendererDeleter {
            void operator()(ImGuiBgfxRenderer* renderer) const {
                delete renderer;
            }
        };
        ImGuiBgfxRenderer(const ImGuiBgfxRenderer&) = delete;
        auto operator=(const ImGuiBgfxRenderer&) -> ImGuiBgfxRenderer& = delete;
        ImGuiBgfxRenderer(ImGuiBgfxRenderer&&) = delete;
        auto operator=(ImGuiBgfxRenderer&&) -> ImGuiBgfxRenderer& = delete;

        using ImGuiBgfxRendererPtr = std::unique_ptr<ImGuiBgfxRenderer, ImGuiBgfxRendererDeleter>;

        static ImGuiBgfxRendererPtr createRenderer();

        auto init() -> std::expected<void, core::EditorError>;
        auto shutdown() -> void;

        auto renderDrawData(
            ImDrawData* drawData,
            bgfx::ViewId viewId,
            engine::graphics::RenderExtent extent
        ) -> void;

        [[nodiscard]] static auto textureId(bgfx::TextureHandle texture) -> ImTextureID;
        [[nodiscard]] static auto textureHandle(ImTextureID textureId) -> bgfx::TextureHandle;

    private:
        ImGuiBgfxRenderer() = default;
        ~ImGuiBgfxRenderer();

        auto processTextureUpdates(ImDrawData* drawData) -> void;
        auto createTexture(ImTextureData& texture) -> bool;
        auto updateTexture(ImTextureData& texture) -> bool;
        auto destroyTexture(ImTextureData& texture) -> void;

        bgfx::ProgramHandle m_program = BGFX_INVALID_HANDLE;
        bgfx::UniformHandle m_sampler = BGFX_INVALID_HANDLE;
        bgfx::VertexLayout m_vertexLayout{};
        bool m_initialized{false};
    };
} // namespace AetherEditor::ui

#endif //SMB_IMGUIBGFXRENDERER_HPP
