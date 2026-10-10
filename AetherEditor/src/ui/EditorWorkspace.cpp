//
// Created by drhaz on 08.10.2026.
//

#include "ui/EditorWorkspace.hpp"


namespace AetherEditor::ui {
    auto EditorWorkspace::create() -> EditorWorkspacePtr {
        return EditorWorkspacePtr(new EditorWorkspace(), EditorWorkspaceDeleter{});
    }

    auto EditorWorkspace::draw(AetherEngine::components::CameraComponent &camera) -> viewport::SceneViewportFrame {
        drawMenuBar();
        drawDockspace();
        return drawPanels(camera);
    }

    auto EditorWorkspace::init() -> std::expected<void, AetherEditor::core::EditorError> {
        m_hierarchy = panels::HierarchyPanel::createHierarchyPanel();
        m_inspector = panels::InspectorPanel::createInspectorPanel();
        m_sceneViewport = panels::SceneViewportPanel::createSceneViewportPanel();
        if (!m_hierarchy || !m_inspector || !m_sceneViewport) {
            return std::unexpected(AetherEditor::core::EditorError{5, "Failed to create panels"});
        }
        return {};
    }

    auto EditorWorkspace::setPanelVisible(EditorWorkspacePanels panel, bool visible) -> std::expected<void, AetherEditor::core::EditorError> {
        switch (panel) {
            case EditorWorkspacePanels::AssetBrowser:
                m_showAssetBrowser = visible;
                break;
            case EditorWorkspacePanels::Console:
                m_showConsole = visible;
                break;
            case EditorWorkspacePanels::Hierarchy:
                m_showHierarchy = visible;
                break;
            case EditorWorkspacePanels::Inspector:
                m_showInspector = visible;
                break;
            case EditorWorkspacePanels::SceneViewport:
                m_showSceneViewport = visible;
                break;
            default:
                return std::unexpected(AetherEditor::core::EditorError{5, "Failed to set panels"});
        }
        return {};
    }

    auto EditorWorkspace::drawDockspace() -> void {
        ImGui::DockSpaceOverViewport(
            0,
            ImGui::GetMainViewport(),
            ImGuiDockNodeFlags_PassthruCentralNode
        );
    }

    auto EditorWorkspace::drawPanels(AetherEngine::components::CameraComponent &camera) -> viewport::SceneViewportFrame {
        if (m_showInspector) {
            m_inspector->draw(m_showInspector);
        }
        if (m_showHierarchy) {
            m_hierarchy->draw(m_showHierarchy);
        }
        if (m_showSceneViewport) {
            return m_sceneViewport->draw(m_showSceneViewport, camera);
        }
        return {};
    }

    auto EditorWorkspace::drawMenuBar() -> void {
        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("View")) {
                ImGui::MenuItem("Asset Browser", nullptr, &m_showAssetBrowser);
                ImGui::MenuItem("Console", nullptr, &m_showConsole);
                ImGui::MenuItem("Hierarchy", nullptr, &m_showHierarchy);
                ImGui::MenuItem("Inspector", nullptr, &m_showInspector);
                ImGui::MenuItem("Scene Viewport", nullptr, &m_showSceneViewport);

                ImGui::EndMenu();
            }

            ImGui::EndMainMenuBar();
        }
    }
} // ui
// AetherEditor