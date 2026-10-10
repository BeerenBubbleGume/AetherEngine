//
// Created by drhaz on 08.10.2026.
//

#ifndef SMB_EDITORWORKSPACE_HPP
#define SMB_EDITORWORKSPACE_HPP
#include <expected>
#include <memory>
#include <imgui.h>

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
        auto draw(AetherEngine::components::CameraComponent &camera) -> viewport::SceneViewportFrame;
        auto init() -> std::expected<void, AetherEditor::core::EditorError>;
        auto setPanelVisible(EditorWorkspacePanels panel, bool visible) -> std::expected<void, AetherEditor::core::EditorError>;
    private:
        EditorWorkspace() = default;
        ~EditorWorkspace() = default;

        auto drawDockspace() -> void;
        auto drawPanels(AetherEngine::components::CameraComponent &camera) -> viewport::SceneViewportFrame;
        auto drawMenuBar() -> void;

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
