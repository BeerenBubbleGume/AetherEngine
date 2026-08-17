//
// Created by drhaz on 09.07.2026.
//

#include "core/AetherEditor.hpp"

#include <imgui.h>

#include "graphics/SceneView.hpp"
#include "math/CameraMatrices.hpp"
#include "systems/RenderSystem.hpp"

auto AetherEditor::core::Editor::init(
    AetherEngine::core::EngineContext &ctx) -> std::expected<void, AetherEngine::core::EngineError> {
    ctx.input.bindKey(AetherEngine::systems::InputAction::Quit, AetherEngine::systems::Key::Escape);

    m_editorCameraTransform.transform.position = {0.0f, 0.0f, 5.0f};
    m_editorCameraTransform.transform.rotation = AetherEngine::math::Quat::identity();
    m_editorCameraTransform.transform.scale = AetherEngine::math::Vec3::one();
    m_editorCamera.fovYDegrees = 60.0f;
    m_editorCamera.nearPlane = 0.1f;
    m_editorCamera.farPlane = 100.0f;
    m_gui = ui::EditorGui::createGui();
    auto guiInitResult = m_gui->init(ctx);
    if (!guiInitResult) {
        return std::unexpected(AetherEngine::core::EngineError{4, guiInitResult.error().msg});
    }
    return {};
}

auto AetherEditor::core::Editor::onEvent(const SDL_Event& event) -> void {
    if (m_gui) {
        m_gui->onEvent(event);
    }
}

auto AetherEditor::core::Editor::update(float dt, AetherEngine::core::EngineContext &ctx) -> void {
    (void)dt;
    if ((!m_gui || !m_gui->wantsKeyboardCapture()) &&
        ctx.input.wasActionPressed(AetherEngine::systems::InputAction::Quit)) {
        ctx.requestQuit();
    }
}

auto AetherEditor::core::Editor::render(AetherEngine::core::EngineContext& ctx) -> void {
    const auto extent = ctx.renderer.backbufferExtent();
    const AetherEngine::graphics::SceneView view{
        .viewId = 10,
        .target = nullptr,
        .viewMatrix = AetherEngine::math::CameraMatrices::makeView(m_editorCameraTransform),
        .projectionMatrix = AetherEngine::math::CameraMatrices::makeProjection(m_editorCamera, extent.width, extent.height),
        .viewport = {
            .width = extent.width,
            .height = extent.height
        },
        .clearFlags = BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH,
        .clearColor = 0x20252bff
    };

    ctx.renderer.renderScene(ctx.scene, ctx.assets, view);

    if (m_gui) {
        m_gui->beginFrame();
        ImGui::Begin("Aether Editor");
        ImGui::TextUnformatted("ImGui / bgfx / SDL3 integration is active.");
        ImGui::Text("Backbuffer: %u x %u", extent.width, extent.height);
        ImGui::End();
        m_gui->endFrame(extent);
    }
}

auto AetherEditor::core::Editor::shutdown(AetherEngine::core::EngineContext& ctx) -> void {
    (void)ctx;
    if (m_gui) {
        m_gui->shutdown();
        m_gui.reset();
    }
}

auto AetherEditor::core::Editor::createEditor() -> AetherEditorPtr {
    return AetherEditorPtr(new Editor());
}

auto AetherEditor::core::Editor::runConfig() const -> AetherEngine::core::EngineRunConfig {
    return {.updatePhysics = false};
}
