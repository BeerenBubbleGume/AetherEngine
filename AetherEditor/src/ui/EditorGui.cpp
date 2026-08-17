//
// Created by drhaz on 10.07.2026.
//

#include "ui/EditorGui.hpp"

#include <imgui.h>
#include <imgui_impl_sdl3.h>

namespace AetherEditor::ui {
    EditorGui::EditorGuiPtr EditorGui::createGui() {
        return EditorGuiPtr(new EditorGui(), EditorGuiDeleter{});
    }

    auto EditorGui::init(const AetherEngine::core::EngineContext &ctx) -> std::expected<void, AetherEditor::core::EditorError> {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();

        auto& io = ImGui::GetIO();

        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

        ImGui::StyleColorsDark();

        if (!ImGui_ImplSDL3_InitForOther(
                ctx.window.getSDLWindow()))
        {
            ImGui::DestroyContext();
            return std::unexpected(
                core::EditorError{1, "Failed to initialize SDL3 backend"}
            );
        }
        m_renderer = ImGuiBgfxRenderer::createRenderer();
        auto rendererResult = m_renderer->init();
        if (!rendererResult) {
            ImGui_ImplSDL3_Shutdown();
            ImGui::DestroyContext();
            return std::unexpected(rendererResult.error());
        }

        m_initialized = true;
        return {};
    }

    auto EditorGui::onEvent(const SDL_Event &event) -> void {
        if (m_initialized) {
            ImGui_ImplSDL3_ProcessEvent(&event);
        }
    }

    auto EditorGui::beginFrame() -> void {
        if (!m_initialized) {
            return;
        }
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();
    }

    auto EditorGui::endFrame(AetherEngine::graphics::RenderExtent backbufferExtent) -> void {
        if (!m_initialized || !m_renderer) {
            return;
        }
        ImGui::Render();

        m_renderer->renderDrawData(
            ImGui::GetDrawData(),
            240,
            backbufferExtent
        );
    }

    auto EditorGui::shutdown() -> void {
        if (!m_initialized) {
            return;
        }

        if (m_renderer) {
            m_renderer->shutdown();
        }
        ImGui_ImplSDL3_Shutdown();
        ImGui::DestroyContext();

        m_initialized = false;
    }

    auto EditorGui::wantsKeyboardCapture() const -> bool {
        return m_initialized && ImGui::GetIO().WantCaptureKeyboard;
    }

    auto EditorGui::wantsMouseCapture() const -> bool {
        return m_initialized && ImGui::GetIO().WantCaptureMouse;
    }

    EditorGui::~EditorGui() {
        shutdown();
    }
} // namespace AetherEditor::ui
