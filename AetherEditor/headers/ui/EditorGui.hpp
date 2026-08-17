//
// Created by drhaz on 10.07.2026.
//

#ifndef SMB_EDITORGUI_HPP
#define SMB_EDITORGUI_HPP
#include <imgui.h>

#include "ImGuiBgfxRenderer.hpp"
#include "core/EngineContext.hpp"
#include "graphics/RenderTarget.hpp"

namespace AetherEditor::ui {
    class EditorGui final {
    public:
        struct EditorGuiDeleter {
            void operator()(EditorGui* gui) const {
                delete gui;
            }
        };
        EditorGui(const EditorGui &) = delete;
        EditorGui(EditorGui &&) = delete;
        auto operator=(const EditorGui &) -> EditorGui & = delete;
        auto operator=(EditorGui &&) -> EditorGui & = delete;

        using EditorGuiPtr = std::unique_ptr<EditorGui, EditorGuiDeleter>;
        static EditorGuiPtr createGui();

        auto init(const AetherEngine::core::EngineContext& ctx) -> std::expected<void, AetherEditor::core::EditorError>;
        auto onEvent(const SDL_Event& event) -> void;
        auto beginFrame() -> void;
        auto endFrame(AetherEngine::graphics::RenderExtent backbufferExtent) -> void;
        auto shutdown() -> void;

        [[nodiscard]] auto wantsKeyboardCapture() const -> bool;
        [[nodiscard]] auto wantsMouseCapture() const -> bool;

    private:
        ~EditorGui();
        EditorGui() = default;
        using ImGuiBgfxRendererPtr = std::unique_ptr<ImGuiBgfxRenderer, ImGuiBgfxRenderer::ImGuiBgfxRendererDeleter>;
        ImGuiBgfxRendererPtr m_renderer;
        bool m_initialized{false};
    };
} // ui

#endif //SMB_EDITORGUI_HPP
