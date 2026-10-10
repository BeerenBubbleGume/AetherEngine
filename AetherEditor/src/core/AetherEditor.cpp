//
// Created by drhaz on 09.07.2026.
//

#include "core/AetherEditor.hpp"

#include <imgui.h>
#include <iostream>

#include "graphics/SceneView.hpp"
#include "math/CameraMatrices.hpp"
#include "systems/RenderSystem.hpp"

auto AetherEditor::core::Editor::init(
    AetherEngine::core::EngineContext &ctx) -> std::expected<void, AetherEngine::core::EngineError> {
    using Action = AetherEngine::systems::InputAction;
    using Key = AetherEngine::systems::Key;
    ctx.input.bindKey(Action::Quit, Key::Escape);
    ctx.input.bindKey(Action::CameraMoveForward, Key::W);
    ctx.input.bindKey(Action::CameraMoveBackward, Key::S);
    ctx.input.bindKey(Action::CameraMoveLeft, Key::A);
    ctx.input.bindKey(Action::CameraMoveRight, Key::D);

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
    m_workspace = ui::EditorWorkspace::create();
    auto workspaceInitResult = m_workspace->init();
    if (!workspaceInitResult) {
        return std::unexpected(AetherEngine::core::EngineError{4, workspaceInitResult.error().msg});
    }
    m_cameraController = viewport::EditorCameraController::createEditorCameraController();
    m_inputRouter = input::EditorInputRouter::createEditorInputRouter();

    return {};
}

auto AetherEditor::core::Editor::onEvent(const SDL_Event& event) -> void {
    if (m_gui) {
        m_gui->onEvent(event);
    }
    if (event.type == SDL_EVENT_WINDOW_FOCUS_LOST) {
        if (m_inputRouter) {
            m_inputRouter->cancel();
        }

        auto* window = SDL_GetWindowFromID(event.window.windowID);
        setRelativeMouseMode(window, false);
    }
}

auto AetherEditor::core::Editor::update(float dt, AetherEngine::core::EngineContext &ctx) -> void {
    m_frameDelta = dt;
    if ((!m_gui || !m_gui->wantsKeyboardCapture()) &&
        ctx.input.wasActionPressed(AetherEngine::systems::InputAction::Quit)) {
        ctx.requestQuit();
    }
}

auto AetherEditor::core::Editor::render(AetherEngine::core::EngineContext& ctx) -> void {
    const auto backbufferExtent = ctx.renderer.backbufferExtent();
    constexpr bgfx::ViewId BackgroundView = 0;

    bgfx::setViewFrameBuffer(BackgroundView, BGFX_INVALID_HANDLE);
    bgfx::setViewRect(
        BackgroundView,
        0, 0,
        backbufferExtent.width,
        backbufferExtent.height
    );
    bgfx::setViewClear(
        BackgroundView,
        BGFX_CLEAR_COLOR,
        0x20252bff
    );
    bgfx::touch(BackgroundView);

    m_gui->beginFrame();
    auto* window = ctx.window.getSDLWindow();

    const bool applicationFocused =
        window != nullptr &&
        (SDL_GetWindowFlags(window) & SDL_WINDOW_INPUT_FOCUS) != 0;

    const auto viewportFrame = m_workspace->draw(m_editorCamera);
    auto cameraInput = m_inputRouter->route(viewportFrame.state, ctx.input, applicationFocused);

    if (!setRelativeMouseMode(window, cameraInput.looking)) {
        m_inputRouter->cancel();
        cameraInput = {};
    }
    if (viewportFrame.target && viewportFrame.state.visible) {
        m_cameraController->dolly(m_editorCameraTransform, m_editorCamera, cameraInput.dolly);
        if (cameraInput.looking) {
            m_cameraController->look(m_editorCameraTransform, cameraInput.lookDelta);
            m_cameraController->move(m_editorCameraTransform, m_editorCamera, cameraInput.moveAxis, m_frameDelta);
        }
        const auto sceneExtent = viewportFrame.target->extent();
        const AetherEngine::graphics::SceneView view{
            .viewId = 10,
            .target = viewportFrame.target,
            .viewMatrix =
                AetherEngine::math::CameraMatrices::makeView(
                    m_editorCameraTransform
                ),
            .projectionMatrix =
                AetherEngine::math::CameraMatrices::makeProjection(
                    m_editorCamera,
                    sceneExtent.width,
                    sceneExtent.height
                ),
            .viewport = {
                .width = sceneExtent.width,
                .height = sceneExtent.height
            },
            .clearFlags = BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH,
            .clearColor = 0x20252bff
        };

        ctx.renderer.renderScene(
            ctx.getScene(),
            ctx.assets,
            ctx.sceneAssetBinding,
            view
        );
    }

    m_gui->endFrame(backbufferExtent);
}

auto AetherEditor::core::Editor::shutdown(AetherEngine::core::EngineContext& ctx) -> void {
    if (m_inputRouter) {
        m_inputRouter->cancel();
    }

    setRelativeMouseMode(ctx.window.getSDLWindow(), false);
    m_workspace.reset();

    if (m_gui) {
        m_gui->shutdown();
        m_gui.reset();
    }
}

auto AetherEditor::core::Editor::saveAs(AetherEngine::core::EngineContext &ctx, std::string_view filename) -> std::expected<void, AetherEngine::core::EngineError> {
    if (!m_document.scene) {
        return std::unexpected(
            AetherEngine::core::EngineError{2, "No scene to save"}
        );
    }
    if (filename.empty()) {
        return std::unexpected(
            AetherEngine::core::EngineError{2, "Scene file name is empty"}
        );
    }

    auto serialized = ctx.sceneSerializer.serializeScene(*m_document.scene, filename);
    if (!serialized) {
        return std::unexpected(AetherEngine::core::EngineError{2, serialized.error().message});
    }
    m_document.fileStem = filename;
    m_document.dirty = false;

    return {};
}

auto AetherEditor::core::Editor::save(AetherEngine::core::EngineContext &ctx) -> std::expected<void, AetherEngine::core::EngineError> {
    auto serialized = ctx.sceneSerializer.serializeScene(*m_document.scene);
    if (!serialized) {
        return std::unexpected(AetherEngine::core::EngineError{2, serialized.error().message});
    }
    m_document.dirty = false;

    return {};
}

auto AetherEditor::core::Editor::load(AetherEngine::core::EngineContext &ctx, std::string_view filename) -> std::expected<void, AetherEngine::core::EngineError> {
    auto loaded = ctx.sceneSerializer.deserializeScene(filename);
    if (!loaded) {
        return std::unexpected(
            AetherEngine::core::EngineError{2, loaded.error().message}
        );
    }

    auto replaced = ctx.replaceScene(std::move(*loaded));
    if (!replaced) {
        return std::unexpected(replaced.error());
    }

    m_document.scene = &ctx.getScene();
    m_document.fileStem = filename;
    m_document.dirty = false;
    return {};
}

auto AetherEditor::core::Editor::newScene(AetherEngine::core::EngineContext &ctx, std::string_view name) -> std::expected<void, AetherEngine::core::EngineError> {
    auto createdScene = AetherEngine::scene::Scene::createScene(name);
    auto replaced = ctx.replaceScene(std::move(createdScene));
    if (!replaced) {
        return std::unexpected(replaced.error());
    }
    m_document.scene = &ctx.getScene();
    m_document.fileStem.clear();
    m_document.dirty = true;
    return {};
}

auto AetherEditor::core::Editor::createEditor() -> AetherEditorPtr {
    return AetherEditorPtr(new Editor());
}

auto AetherEditor::core::Editor::runConfig() const -> AetherEngine::core::EngineRunConfig {
    return {.updatePhysics = false};
}

auto AetherEditor::core::Editor::setRelativeMouseMode(SDL_Window *window, bool enabled) -> bool {
    if (m_relativeMouseMode == enabled) {
        return true;
    }

    if (window == nullptr) {
        return false;
    }

    if (!SDL_SetWindowRelativeMouseMode(window, enabled)) {
        SDL_Log(
            "Failed to set relative mouse mode: %s",
            SDL_GetError()
        );
        return false;
    }

    m_relativeMouseMode = enabled;
    return true;
}
