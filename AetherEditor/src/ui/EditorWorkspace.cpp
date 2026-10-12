//
// Created by drhaz on 08.10.2026.
//

#include "ui/EditorWorkspace.hpp"


namespace AetherEditor::ui {
    auto EditorWorkspace::create() -> EditorWorkspacePtr {
        return EditorWorkspacePtr(new EditorWorkspace(), EditorWorkspaceDeleter{});
    }

    auto EditorWorkspace::draw(AetherEngine::components::CameraComponent &camera, AetherEngine::scene::Scene &scene, AetherEditor::core::EditorSession& session, commands
                               ::CommandHistory &history) -> EditorWorkspaceFrame {
        EditorWorkspaceFrame frame{};

        drawMenuBar(history, frame);
        drawDockspace();

        auto panels = drawPanels(camera, scene, session, history);
        frame.viewport = panels.viewport;
        frame.sceneChanged = panels.sceneChanged;
        processHistoryShortcuts(history, frame);
        return frame;
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

    auto EditorWorkspace::finishPendingEdits(AetherEngine::scene::Scene &scene,
        commands::CommandHistory &history) const -> void {
        m_inspector->finishTransformEdit(scene, history);
    }

    auto EditorWorkspace::drawDockspace() -> void {
        ImGui::DockSpaceOverViewport(
            0,
            ImGui::GetMainViewport(),
            ImGuiDockNodeFlags_PassthruCentralNode
        );
    }

    auto EditorWorkspace::drawPanels(AetherEngine::components::CameraComponent &camera, AetherEngine::scene::Scene &scene, core::EditorSession &session, commands
                                     ::CommandHistory &history) -> EditorWorkspaceFrame {
        EditorWorkspaceFrame frame{};

        if (m_showHierarchy) {
            m_hierarchy->draw(m_showHierarchy, scene, session);
        }
        if (m_showInspector) {
            frame.sceneChanged =
                m_inspector->draw(m_showInspector, scene, session, history);
        } else {
            m_inspector->finishTransformEdit(scene, history);
        }
        if (m_showSceneViewport) {
            frame.viewport = m_sceneViewport->draw(m_showSceneViewport, camera);
        }
        return frame;
    }

    auto EditorWorkspace::drawMenuBar(const commands::CommandHistory &history, EditorWorkspaceFrame &frame) -> void {
        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("View")) {
                ImGui::MenuItem("Asset Browser", nullptr, &m_showAssetBrowser);
                ImGui::MenuItem("Console", nullptr, &m_showConsole);
                ImGui::MenuItem("Hierarchy", nullptr, &m_showHierarchy);
                ImGui::MenuItem("Inspector", nullptr, &m_showInspector);
                ImGui::MenuItem("Scene Viewport", nullptr, &m_showSceneViewport);

                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Edit")) {
                frame.undoRequested = ImGui::MenuItem(
                    "Undo", "Ctrl+Z", false,
                    history.canUndo() || m_inspector->hasPendingEdit()
                );

                frame.redoRequested = ImGui::MenuItem(
                    "Redo", "Ctrl+Y", false,
                    history.canRedo() && !m_inspector->hasPendingEdit()
                );

                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }
    }

    auto EditorWorkspace::processHistoryShortcuts(const commands::CommandHistory &history,
        EditorWorkspaceFrame &frame) const -> void {
        constexpr ImGuiInputFlags flags = ImGuiInputFlags_RouteGlobal;

        const bool undoPressed = ImGui::Shortcut(ImGuiMod_Ctrl  | ImGuiKey_Z, flags);
        const bool redoPressed = ImGui::Shortcut(ImGuiMod_Ctrl  | ImGuiKey_Y, flags);

        const bool redoShiftZPressed = ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_Y, flags);

        if (frame.undoRequested || frame.redoRequested) {
            return;
        }
        if (ImGui::IsAnyItemActive() || m_inspector->hasPendingEdit()) {
            return;
        }
        if (undoPressed && history.canUndo()) {
            frame.undoRequested = true;
        } else if ((redoPressed || redoShiftZPressed) && history.canRedo()) {
            frame.redoRequested = true;
        }
    }
} // ui
// AetherEditor