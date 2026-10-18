//
// Created by drhaz on 09.07.2026.
//

#include "core/AetherEditor.hpp"

#include "commands/EntityCommands.hpp"

#include <SDL3/SDL_messagebox.h>
#include <utility>
#include "assets/AssetCatalogService.hpp"

#include "graphics/SceneView.hpp"
#include "math/CameraMatrices.hpp"
#include "systems/RenderSystem.hpp"

namespace {
    auto showEditorError(
        const char* title,
        const AetherEngine::core::EngineError& error,
        SDL_Window* window
    ) -> void {
        SDL_ShowSimpleMessageBox(
            SDL_MESSAGEBOX_ERROR,
            title,
            error.message.c_str(),
            window
        );
    }
}

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
    scanTextures(ctx);
    ctx.logger.info("Editor initialized", "Editor");
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
    renderBackground(backbufferExtent);

    m_gui->beginFrame();
    auto* window = ctx.window.getSDLWindow();
    const bool applicationFocused =
        window != nullptr &&
        (SDL_GetWindowFlags(window) & SDL_WINDOW_INPUT_FOCUS) != 0;

    const auto frame = m_workspace->draw(
        m_editorCamera,
        ctx.getScene(),
        m_session,
        m_history,
        m_documentController->getDocument(),
        ctx.logger, ctx.assets.registry()
    );

    processWorkspaceActions(ctx, frame);
    updateEditorCamera(ctx, frame.viewport, applicationFocused);
    renderSceneViewport(ctx, frame.viewport);

    m_gui->endFrame(backbufferExtent);
}

auto AetherEditor::core::Editor::renderBackground(AetherEngine::graphics::RenderExtent extent) -> void {
    constexpr bgfx::ViewId BackgroundView = 0;

    bgfx::setViewFrameBuffer(BackgroundView, BGFX_INVALID_HANDLE);
    bgfx::setViewRect(
        BackgroundView,
        0, 0,
        extent.width,
        extent.height
    );
    bgfx::setViewClear(
        BackgroundView,
        BGFX_CLEAR_COLOR,
        0x20252bff
    );
    bgfx::touch(BackgroundView);
}

auto AetherEditor::core::Editor::processWorkspaceActions(
    AetherEngine::core::EngineContext& ctx,
    const ui::EditorWorkspaceFrame& frame
) -> void {
    if (frame.refreshSceneListRequested) {
        refreshSceneList(ctx);
    }

    processAssetActions(ctx, frame.assets);

    if (frame.renameEntityRequested) {
        renameEntity(ctx, *frame.renameEntityRequested);
    }
    if (frame.createEntityRequested) {
        createEntity(ctx);
    }
    processHistoryActions(ctx, frame);
    processSaveRequests(ctx, frame);
    processDocumentActions(ctx, frame);
    updateDocumentDirty(ctx);
}

auto AetherEditor::core::Editor::refreshSceneList(AetherEngine::core::EngineContext& ctx) -> void {
    auto scenes = docs::DocumentController::listScenes(ctx);
    if (scenes) {
        m_workspace->setAvailableScenes(std::move(*scenes));
    } else {
        m_workspace->setAvailableScenes({});
        showEditorError("Scene list failed", scenes.error(), ctx.window.getSDLWindow());
    }
}

auto AetherEditor::core::Editor::scanTextures(AetherEngine::core::EngineContext& ctx) -> void {
    const auto report = assets::AssetCatalogService::scanTextures(ctx.assets);
    if (!report) {
        ctx.logger.error(report.error().message, "Assets");
        m_workspace->setAssetStatus(report.error().message, true);
        return;
    }
    for (const auto& warning : report->warnings) ctx.logger.warning(warning, "Assets");
    const auto message = "Textures added: " + std::to_string(report->added) +
        "; already registered: " + std::to_string(report->existing) +
        "; skipped: " + std::to_string(report->warnings.size());
    ctx.logger.info(message, "Assets");
    m_workspace->setAssetStatus(message, false);
}

auto AetherEditor::core::Editor::processAssetActions(
    AetherEngine::core::EngineContext& ctx, const panels::AssetBrowserFrame& frame
) -> void {
    if (frame.scanTexturesRequested) scanTextures(ctx);
    if (frame.createMaterial) {
        const auto created = assets::AssetCatalogService::createMaterial(ctx.assets, *frame.createMaterial);
        if (created) {
            const auto message = "Material created: " + frame.createMaterial->name;
            ctx.logger.info(message, "Assets");
            m_workspace->setAssetStatus(message, false);
            m_workspace->materialCreated(*created);
        } else {
            ctx.logger.error(created.error().message, "Assets");
            m_workspace->setAssetStatus(created.error().message, true);
        }
    }
}

auto AetherEditor::core::Editor::createEntity(AetherEngine::core::EngineContext& ctx) -> void {
    auto& scene = ctx.getScene();
    m_workspace->finishPendingEdits(scene, m_history);

    std::string name = "Entity";
    for (std::size_t suffix = 1; scene.findEntityByName(name); ++suffix) {
        name = "Entity (" + std::to_string(suffix) + ")";
    }
    auto entityId = AetherEngine::scene::generateEntityId();
    while (!entityId.isValid() || scene.findEntityById(entityId)) {
        entityId = AetherEngine::scene::generateEntityId();
    }

    if (m_history.execute(std::make_unique<commands::CreateEntityCommand>(entityId, name), scene)) {
        m_session.selectedEntity = entityId;
    }
}

auto AetherEditor::core::Editor::renameEntity(
    AetherEngine::core::EngineContext& ctx, const panels::RenameEntityRequest& request
) -> void {
    auto& scene = ctx.getScene();
    const auto entity = scene.findEntityById(request.entityId);
    if (!entity) {
        return;
    }
    const auto& before = scene.getComponent<AetherEngine::components::IdentityComponent>(*entity).name;
    if (before == request.name) {
        return;
    }
    m_workspace->finishPendingEdits(scene, m_history);
    m_history.execute(std::make_unique<commands::RenameEntityCommand>(request.entityId, before, request.name), scene);
}

auto AetherEditor::core::Editor::processHistoryActions(
    AetherEngine::core::EngineContext& ctx,
    const ui::EditorWorkspaceFrame& frame
) -> void {
    if (frame.undoRequested) {
        m_workspace->finishPendingEdits(ctx.getScene(), m_history);
        m_history.undo(ctx.getScene());
    } else if (frame.redoRequested) {
        m_workspace->finishPendingEdits(ctx.getScene(), m_history);
        m_history.redo(ctx.getScene());
    }
    if (m_session.selectedEntity && !ctx.getScene().findEntityById(*m_session.selectedEntity)) {
        m_session.selectedEntity.reset();
    }
}

auto AetherEditor::core::Editor::processSaveRequests(
    AetherEngine::core::EngineContext& ctx,
    const ui::EditorWorkspaceFrame& frame
) -> void {
    if (frame.saveAsFileStem) {
        m_workspace->finishPendingEdits(ctx.getScene(), m_history);
        const auto result = saveAs(ctx, *frame.saveAsFileStem);
        if (result) {
            m_workspace->requestCloseSaveAs();
        } else {
            showEditorError("Save As failed", result.error(), ctx.window.getSDLWindow());
        }
        return;
    }

    if (!frame.saveRequested) {
        return;
    }

    m_workspace->finishPendingEdits(ctx.getScene(), m_history);
    if (m_documentController->getDocument().fileStem.empty()) {
        m_workspace->requestOpenSaveAs();
        return;
    }

    const auto result = save(ctx);
    if (!result) {
        showEditorError("Save failed", result.error(), ctx.window.getSDLWindow());
    }
}

auto AetherEditor::core::Editor::processDocumentActions(
    AetherEngine::core::EngineContext& ctx,
    const ui::EditorWorkspaceFrame& frame
) -> void {
    updateDocumentDirty(ctx);
    if (frame.documentActionRequested) {
        m_workspace->finishPendingEdits(ctx.getScene(), m_history);
        updateDocumentDirty(ctx);
        m_pendingDocumentAction = *frame.documentActionRequested;

        if (m_documentController->getDocument().dirty) {
            m_workspace->requestConfirmLoad();
        }
    }

    const bool actionConfirmed = frame.loadDecision == SceneLoadDecision::DiscardChanges;
    const bool actionWithoutConfirmation =
        frame.documentActionRequested.has_value() && !m_documentController->getDocument().dirty;

    if (frame.loadDecision == SceneLoadDecision::Cancel) {
        m_pendingDocumentAction.reset();
        return;
    }
    if (!m_pendingDocumentAction || (!actionConfirmed && !actionWithoutConfirmation)) {
        return;
    }

    const auto action = *m_pendingDocumentAction;
    std::expected<void, AetherEngine::core::EngineError> result;
    switch (action.kind) {
        case DocumentActionKind::NewScene:
            result = newScene(ctx, action.sceneName);
            break;

        case DocumentActionKind::OpenScene:
            result = load(ctx, action.sceneName);
            break;
    }

    if (result) {
        if (action.kind == DocumentActionKind::OpenScene) {
            m_workspace->requestCloseLoad();
        }
    } else {
        showEditorError("Scene action failed", result.error(), ctx.window.getSDLWindow());
    }

    m_pendingDocumentAction.reset();
}

auto AetherEditor::core::Editor::updateEditorCamera(
    AetherEngine::core::EngineContext& ctx,
    const viewport::SceneViewportFrame& frame,
    bool applicationFocused
) -> void {
    auto cameraInput = m_inputRouter->route(frame.state, ctx.input, applicationFocused);

    if (!setRelativeMouseMode(ctx.window.getSDLWindow(), cameraInput.looking)) {
        m_inputRouter->cancel();
        cameraInput = {};
    }
    if (!frame.target || !frame.state.visible) {
        return;
    }

    m_cameraController->dolly(m_editorCameraTransform, m_editorCamera, cameraInput.dolly);
    if (cameraInput.looking) {
        m_cameraController->look(m_editorCameraTransform, cameraInput.lookDelta);
        m_cameraController->move(m_editorCameraTransform, m_editorCamera, cameraInput.moveAxis, m_frameDelta);
    }
}

auto AetherEditor::core::Editor::renderSceneViewport(
    AetherEngine::core::EngineContext& ctx,
    const viewport::SceneViewportFrame& frame
) const -> void {
    if (!frame.target || !frame.state.visible) {
        return;
    }

    const auto sceneExtent = frame.target->extent();
    const AetherEngine::graphics::SceneView view{
        .viewId = 10,
        .target = frame.target,
        .viewMatrix = AetherEngine::math::CameraMatrices::makeView(m_editorCameraTransform),
        .projectionMatrix = AetherEngine::math::CameraMatrices::makeProjection(
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

    ctx.renderer.renderScene(ctx.getScene(), ctx.assets, ctx.sceneAssetBinding, view);
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
