//
// Created by drhaz on 08.10.2026.
//

#ifndef SMB_EDITORWORKSPACE_HPP
#define SMB_EDITORWORKSPACE_HPP
#include <expected>
#include <memory>
#include <imgui.h>
#include <imgui_stdlib.h>

#include "assets/AssetRegistry.hpp"
#include "core/EditorSession.hpp"
#include "core/ETypes.hpp"
#include "panels/InspectorPanel.hpp"
#include "panels/HierarchyPanel.hpp"
#include "panels/SceneViewportPanel.hpp"
#include "panels/ConsolePanel.hpp"
#include "core/ETypes.hpp"
#include "documents/SceneEditorDocument.hpp"
#include "panels/AssetBrowserPanel.hpp"

namespace AetherEditor::ui {
    struct EditorWorkspaceFrame {
        viewport::SceneViewportFrame viewport{};
        panels::AssetBrowserFrame assets{};
        bool sceneChanged = false;
        bool createEntityRequested = false;
        std::optional<panels::RenameEntityRequest> renameEntityRequested;
        bool undoRequested = false;
        bool redoRequested = false;
        bool saveRequested = false;
        std::optional<std::string> saveAsFileStem;
        bool refreshSceneListRequested = false;
        core::SceneLoadDecision loadDecision = core::SceneLoadDecision::None;
        std::optional<core::PendingDocumentAction> documentActionRequested;
    };
    class EditorWorkspace {
    public:
        struct EditorWorkspaceDeleter {
            void operator()(EditorWorkspace* workspace) const {
                delete workspace;
            }
        };
        using EditorWorkspacePtr = std::unique_ptr<EditorWorkspace, EditorWorkspaceDeleter>;
        EditorWorkspace(const EditorWorkspace&) = delete;
        EditorWorkspace& operator=(const EditorWorkspace&) = delete;
        EditorWorkspace(EditorWorkspace&&) = delete;
        EditorWorkspace& operator=(EditorWorkspace&&) = delete;
        static auto create() -> EditorWorkspacePtr;
        auto draw(AetherEngine::components::CameraComponent &camera, AetherEngine::scene::Scene &scene, AetherEditor::core::EditorSession& session, commands
                  ::CommandHistory &history, const docs::SceneEditorDocument &document, const AetherEngine::logging::Logger &logger, const
                  AetherEngine::assets::AssetRegistry &registry) -> EditorWorkspaceFrame;
        auto init() -> std::expected<void, AetherEditor::core::EditorError>;
        auto setPanelVisible(core::EditorWorkspacePanels panel, bool visible) -> std::expected<void, AetherEditor::core::EditorError>;
        auto finishPendingEdits(AetherEngine::scene::Scene& scene, commands::CommandHistory& history) const -> void;
        auto requestOpenSaveAs() -> void;
        auto requestCloseSaveAs() -> void;
        auto requestCloseLoad() -> void;
        auto requestConfirmLoad() -> void;
        auto setAvailableScenes(std::vector<std::string> scenes) -> void;
        auto setAssetStatus(std::string message, bool error) -> void;
        auto materialCreated(AetherEngine::assets::AssetId id) -> void;

        [[nodiscard]] auto hasPendingChanges(AetherEngine::scene::Scene& scene) const -> bool;
    private:
        EditorWorkspace() = default;
        ~EditorWorkspace() = default;

        auto drawDockspace() -> void;
        auto drawPanels(AetherEngine::components::CameraComponent &camera, AetherEngine::scene::Scene &scene, core::EditorSession &session, commands
                        ::CommandHistory &history, const AetherEngine::logging::Logger &logger, const AetherEngine::assets::AssetRegistry &
                        registry) -> EditorWorkspaceFrame;
        auto drawMenuBar(const commands::CommandHistory &history, EditorWorkspaceFrame &frame, const docs::SceneEditorDocument &document) -> void;
        auto processHistoryShortcuts(const commands::CommandHistory& history, EditorWorkspaceFrame& frame, const docs::SceneEditorDocument &document) const -> void;

        using InspectorPtr = panels::InspectorPanel::InspectorPanelPtr;
        using HierarchyPtr = panels::HierarchyPanel::HierarchyPanelPtr;
        using SceneViewportPtr = panels::SceneViewportPanel::SceneViewportPanelPtr;
        using ConsolePtr = panels::ConsolePanel::ConsolePanelPtr;
        using AssetBrowserPtr = panels::AssetBrowserPanel::AssetBrowserPanelPtr;
        InspectorPtr m_inspector;
        HierarchyPtr m_hierarchy;
        SceneViewportPtr m_sceneViewport;
        ConsolePtr m_console;
        AssetBrowserPtr m_assetBrowser;

        bool m_showHierarchy = true;
        bool m_showInspector = true;
        bool m_showSceneViewport = true;
        bool m_showAssetBrowser = true;
        bool m_showConsole = true;

        bool m_openSaveAs = false;
        bool m_closeSaveAs = false;
        char m_saveAsName[129]{};

        bool m_openLoad = false;
        bool m_closeLoad = false;
        bool m_openLoadConfirmation = false;
        std::string m_loadName;
        std::vector<std::string> m_availableScenes;
    };

} // ui
// AetherEditor

#endif //SMB_EDITORWORKSPACE_HPP
