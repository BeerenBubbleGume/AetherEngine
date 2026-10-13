//
// Created by drhaz on 09.07.2026.
//

#include "core/AetherEditor.hpp"

#include <imgui.h>
#include <iostream>
#include <SDL3/SDL_messagebox.h>

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
    m_documentController->setScene(ctx.getScene());
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

    const auto workspaceFrame = m_workspace->draw(m_editorCamera, ctx.getScene(), m_session, m_history, m_documentController->getDocument());
    if (workspaceFrame.refreshSceneListRequested) {
        auto scenes = docs::DocumentController::listScenes(ctx);

        if (scenes) {
            m_workspace->setAvailableScenes(std::move(*scenes));
        } else {
            m_workspace->setAvailableScenes({});

            SDL_ShowSimpleMessageBox(
                SDL_MESSAGEBOX_ERROR,
                "Scene list failed",
                scenes.error().message.c_str(),
                ctx.window.getSDLWindow()
            );
        }
    }
    if (workspaceFrame.undoRequested) {
        m_workspace->finishPendingEdits(ctx.getScene(), m_history);
        m_history.undo(ctx.getScene());
    } else if (workspaceFrame.redoRequested) {
        m_workspace->finishPendingEdits(ctx.getScene(), m_history);
        m_history.redo(ctx.getScene());
    }
    if (workspaceFrame.saveAsFileStem) {
        m_workspace->finishPendingEdits(ctx.getScene(), m_history);

        const auto result =
            saveAs(ctx, *workspaceFrame.saveAsFileStem);

        if (result) {
            m_workspace->requestCloseSaveAs();
        } else {
            SDL_ShowSimpleMessageBox(
                SDL_MESSAGEBOX_ERROR,
                "Save As failed",
                result.error().message.c_str(),
                ctx.window.getSDLWindow()
            );
        }
    } else if (workspaceFrame.saveRequested) {
        m_workspace->finishPendingEdits(ctx.getScene(), m_history);

        const auto result = save(ctx);

        if (!result) {
            SDL_ShowSimpleMessageBox(
                SDL_MESSAGEBOX_ERROR,
                "Save failed",
                result.error().message.c_str(),
                ctx.window.getSDLWindow()
            );
        }
    }
    updateDocumentDirty(ctx);

    if (workspaceFrame.loadFileStem) {
        m_workspace->finishPendingEdits(ctx.getScene(), m_history);
        updateDocumentDirty(ctx);
        m_pendingLoadFileStem = *workspaceFrame.loadFileStem;

        if (m_documentController->getDocument().dirty) {
            m_workspace->requestConfirmLoad();
        }
    }

    const bool loadConfirmed =
        workspaceFrame.loadDecision == ui::SceneLoadDecision::DiscardChanges;
    const bool loadWithoutConfirmation =
        workspaceFrame.loadFileStem && !m_documentController->getDocument().dirty;

    if (workspaceFrame.loadDecision == ui::SceneLoadDecision::Cancel) {
        m_pendingLoadFileStem.reset();
    } else if (m_pendingLoadFileStem && (loadConfirmed || loadWithoutConfirmation)) {
        const auto result = load(ctx, *m_pendingLoadFileStem);
        if (result) {
            m_workspace->requestCloseLoad();
        } else {
            SDL_ShowSimpleMessageBox(
                SDL_MESSAGEBOX_ERROR,
                "Load failed",
                result.error().message.c_str(),
                ctx.window.getSDLWindow()
            );
        }
        m_pendingLoadFileStem.reset();
    }
    updateDocumentDirty(ctx);
    const auto& viewportFrame = workspaceFrame.viewport;

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
    auto result = m_documentController->saveAs(ctx, filename);
    if (result) {
        m_history.markSaved();
    }
    return result;
}

auto AetherEditor::core::Editor::save(AetherEngine::core::EngineContext &ctx) -> std::expected<void, AetherEngine::core::EngineError> {
    auto result = m_documentController->save(ctx);
    if (result) {
        m_history.markSaved();
    }
    return result;
}

auto AetherEditor::core::Editor::load(AetherEngine::core::EngineContext &ctx, std::string_view filename) -> std::expected<void, AetherEngine::core::EngineError> {

    if (m_workspace) {
        m_workspace->finishPendingEdits(ctx.getScene(), m_history);
    }
    auto loadResult = m_documentController->load(ctx, filename);
    if (!loadResult) {
        return std::unexpected(loadResult.error());
    }
    m_session.selectedEntity.reset();
    m_history.clear();
    return {};
}

auto AetherEditor::core::Editor::newScene(AetherEngine::core::EngineContext &ctx, std::string_view name) -> std::expected<void, AetherEngine::core::EngineError> {
    if (m_workspace) {
        m_workspace->finishPendingEdits(ctx.getScene(), m_history);
    }
    auto newSceneResult = m_documentController->newScene(ctx, name);
    if (!newSceneResult) {
        return std::unexpected(newSceneResult.error());
    }
    m_session.selectedEntity.reset();
    m_history.clear();
    return {};
}

auto AetherEditor::core::Editor::updateDocumentDirty(AetherEngine::core::EngineContext& ctx) -> void {
    const auto& document = m_documentController->getDocument();
    m_documentController->dirty(
        document.scene != nullptr &&
        (
            document.fileStem.empty() ||
            m_history.isDirty() ||
            m_workspace->hasPendingChanges(ctx.getScene())
        )
    );
}

auto AetherEditor::core::Editor::createEditor(std::string_view initialFileStem) -> AetherEditorPtr {
    auto editor = AetherEditorPtr(new Editor());
    editor->m_documentController = docs::DocumentController::createDocumentController();
    editor->m_documentController->setFileStem(initialFileStem);
    return editor;
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
