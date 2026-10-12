//
// Created by drhaz on 08.10.2026.
//

#ifndef SMB_EDITORWORKSPACE_HPP
#define SMB_EDITORWORKSPACE_HPP
#include <expected>
#include <memory>
#include <imgui.h>

#include "core/EditorSession.hpp"
#include "core/ETypes.hpp"
#include "panels/InspectorPanel.hpp"
#include "panels/HierarchyPanel.hpp"
#include "panels/SceneViewportPanel.hpp"

namespace AetherEditor::ui {
    enum class EditorWorkspacePanels {
        AssetBrowser,
        Console,
        Hierarchy,
        Inspector,
        SceneViewport
    };
    struct EditorWorkspaceFrame {
        viewport::SceneViewportFrame viewport{};
        bool sceneChanged = false;
        bool undoRequested = false;
        bool redoRequested = false;
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
                  ::CommandHistory &history) -> EditorWorkspaceFrame;
        auto init() -> std::expected<void, AetherEditor::core::EditorError>;
        auto setPanelVisible(EditorWorkspacePanels panel, bool visible) -> std::expected<void, AetherEditor::core::EditorError>;
        auto finishPendingEdits(AetherEngine::scene::Scene& scene, commands::CommandHistory& history) const -> void;
    private:
        EditorWorkspace() = default;
        ~EditorWorkspace() = default;

        auto drawDockspace() -> void;
        auto drawPanels(AetherEngine::components::CameraComponent &camera, AetherEngine::scene::Scene &scene, core::EditorSession &session, commands
                        ::CommandHistory &history) -> EditorWorkspaceFrame;
        auto drawMenuBar(const commands::CommandHistory &history, EditorWorkspaceFrame &frame) -> void;
        auto processHistoryShortcuts(const commands::CommandHistory& history, EditorWorkspaceFrame& frame) const -> void;

        using InspectorPtr = panels::InspectorPanel::InspectorPanelPtr;
        using HierarchyPtr = panels::HierarchyPanel::HierarchyPanelPtr;
        using SceneViewportPtr = panels::SceneViewportPanel::SceneViewportPanelPtr;

        InspectorPtr m_inspector;
        HierarchyPtr m_hierarchy;
        SceneViewportPtr m_sceneViewport;

        bool m_showHierarchy = true;
        bool m_showInspector = true;
        bool m_showSceneViewport = true;
        bool m_showAssetBrowser = true;
        bool m_showConsole = true;
    };

} // ui
// AetherEditor

#endif //SMB_EDITORWORKSPACE_HPP
