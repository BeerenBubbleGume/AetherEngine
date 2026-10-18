//
// Created by drhaz on 09.07.2026.
//

#ifndef SMB_AETHEREDITOR_HPP
#define SMB_AETHEREDITOR_HPP
#include <memory>
#include <optional>
#include <string>
#include <string_view>

#include "components/CameraComponent.hpp"
#include "components/TransformComponent.hpp"
#include "core/IApplication.hpp"
#include "ETypes.hpp"
#include <ui/EditorGui.hpp>
#include <ui/EditorWorkspace.hpp>
#include <input/EditorInputRouter.hpp>
#include <viewport/EditorCameraController.hpp>

#include "EditorSession.hpp"
#include "commands/CommandHistory.hpp"
#include "documents/DocumentController.hpp"
#include "documents/SceneEditorDocument.hpp"

namespace AetherEditor::core {
    class Editor final : public AetherEngine::core::IApplication {
    public:
        using AetherEditorPtr = std::unique_ptr<Editor>;

        ~Editor() override = default;

        auto init(AetherEngine::core::EngineContext &ctx) -> std::expected<void, AetherEngine::core::EngineError> override;
        auto onEvent(const SDL_Event& event) -> void override;
        auto update(float dt, AetherEngine::core::EngineContext &ctx) -> void override;
        auto render(AetherEngine::core::EngineContext& ctx) -> void override;
        auto shutdown(AetherEngine::core::EngineContext& ctx) -> void override;

        [[nodiscard]] static auto createEditor(std::string_view initialFileStem) -> AetherEditorPtr;
        [[nodiscard]] auto runConfig() const -> AetherEngine::core::EngineRunConfig override;
    private:
        static auto renderBackground(AetherEngine::graphics::RenderExtent extent) -> void;
        auto processWorkspaceActions(AetherEngine::core::EngineContext& ctx, const ui::EditorWorkspaceFrame& frame) -> void;
        auto refreshSceneList(AetherEngine::core::EngineContext& ctx) -> void;
        auto scanTextures(AetherEngine::core::EngineContext& ctx) -> void;
        auto processAssetActions(AetherEngine::core::EngineContext& ctx, const panels::AssetBrowserFrame& frame) -> void;
        auto createEntity(AetherEngine::core::EngineContext& ctx) -> void;
        auto renameEntity(AetherEngine::core::EngineContext& ctx, const panels::RenameEntityRequest& request) -> void;
        auto processHistoryActions(AetherEngine::core::EngineContext& ctx, const ui::EditorWorkspaceFrame& frame) -> void;
        auto processSaveRequests(AetherEngine::core::EngineContext& ctx, const ui::EditorWorkspaceFrame& frame) -> void;
        auto processDocumentActions(AetherEngine::core::EngineContext& ctx, const ui::EditorWorkspaceFrame& frame) -> void;
        auto updateEditorCamera(AetherEngine::core::EngineContext& ctx, const viewport::SceneViewportFrame& frame, bool applicationFocused) -> void;
        auto renderSceneViewport(AetherEngine::core::EngineContext& ctx, const viewport::SceneViewportFrame& frame) const -> void;

        auto saveAs(AetherEngine::core::EngineContext& ctx, std::string_view filename) -> std::expected<void, AetherEngine::core::EngineError>;
        auto save(AetherEngine::core::EngineContext& ctx) -> std::expected<void, AetherEngine::core::EngineError>;
        auto load(AetherEngine::core::EngineContext& ctx, std::string_view filename) -> std::expected<void, AetherEngine::core::EngineError>;
        auto newScene(AetherEngine::core::EngineContext& ctx, std::string_view name) -> std::expected<void, AetherEngine::core::EngineError>;
        auto updateDocumentDirty(AetherEngine::core::EngineContext& ctx) -> void;
        Editor() = default;
        using EditorGuiPtr = ui::EditorGui::EditorGuiPtr;
        using EditorWorkspacePtr = ui::EditorWorkspace::EditorWorkspacePtr;
        using InputRouterPtr  = input::EditorInputRouter::EditorInputRouterPtr;
        using CameraControllerPtr = viewport::EditorCameraController::EditorCameraControllerPtr;
        using DocumentControllerPtr = docs::DocumentController::DocumentControllerPtr;
        AetherEngine::components::TransformComponent m_editorCameraTransform{};
        AetherEngine::components::CameraComponent m_editorCamera{};

        EditorGuiPtr m_gui;
        EditorWorkspacePtr m_workspace;
        InputRouterPtr m_inputRouter;
        CameraControllerPtr m_cameraController;
        DocumentControllerPtr m_documentController;
        EditorSession m_session{};
        commands::CommandHistory m_history{};
        std::optional<PendingDocumentAction> m_pendingDocumentAction;
        float m_frameDelta = 0.0f;
        bool m_relativeMouseMode = false;

        auto setRelativeMouseMode(SDL_Window* window, bool enabled) -> bool;
    };
}


#endif //SMB_AETHEREDITOR_HPP
