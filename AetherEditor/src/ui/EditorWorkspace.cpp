//
// Created by drhaz on 08.10.2026.
//

#include "ui/EditorWorkspace.hpp"


namespace AetherEditor::ui {
    auto EditorWorkspace::create() -> EditorWorkspacePtr {
        return EditorWorkspacePtr(new EditorWorkspace(), EditorWorkspaceDeleter{});
    }

    auto EditorWorkspace::draw(AetherEngine::components::CameraComponent &camera, AetherEngine::scene::Scene &scene, AetherEditor::core::EditorSession& session, commands
                               ::CommandHistory &history, const docs::SceneEditorDocument &document, const AetherEngine::logging::Logger &logger, const
                               AetherEngine::assets::AssetRegistry &registry) -> EditorWorkspaceFrame {
        EditorWorkspaceFrame frame{};

        drawMenuBar(history, frame, document);
        drawDockspace();

        auto panels = drawPanels(camera, scene, session, history, logger, registry);
        frame.viewport = panels.viewport;
        frame.sceneChanged = panels.sceneChanged;
        frame.createEntityRequested = panels.createEntityRequested;
        frame.renameEntityRequested = std::move(panels.renameEntityRequested);
        frame.assets = std::move(panels.assets);
        processHistoryShortcuts(history, frame, document);
        return frame;
    }

    auto EditorWorkspace::init() -> std::expected<void, AetherEditor::core::EditorError> {
        m_hierarchy = panels::HierarchyPanel::createHierarchyPanel();
        m_inspector = panels::InspectorPanel::createInspectorPanel();
        m_sceneViewport = panels::SceneViewportPanel::createSceneViewportPanel();
        m_console = panels::ConsolePanel::createConsolePanel();
        m_assetBrowser = panels::AssetBrowserPanel::createAssetBrowserPanel();
        if (!m_hierarchy || !m_inspector || !m_sceneViewport || !m_console || !m_assetBrowser) {
            return std::unexpected(AetherEditor::core::EditorError{5, "Failed to create panels"});
        }
        return {};
    }

    auto EditorWorkspace::setPanelVisible(core::EditorWorkspacePanels panel, bool visible) -> std::expected<void, AetherEditor::core::EditorError> {
        switch (panel) {
            case core::EditorWorkspacePanels::AssetBrowser:
                m_showAssetBrowser = visible;
                break;
            case core::EditorWorkspacePanels::Console:
                m_showConsole = visible;
                break;
            case core::EditorWorkspacePanels::Hierarchy:
                m_showHierarchy = visible;
                break;
            case core::EditorWorkspacePanels::Inspector:
                m_showInspector = visible;
                break;
            case core::EditorWorkspacePanels::SceneViewport:
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

    auto EditorWorkspace::requestOpenSaveAs() -> void {
        m_openSaveAs = true;
    }

    auto EditorWorkspace::setAssetStatus(std::string message, bool error) -> void {
        m_assetBrowser->setStatus(std::move(message), error);
    }

    auto EditorWorkspace::materialCreated(AetherEngine::assets::AssetId id) -> void {
        m_assetBrowser->materialCreated(id);
    }

    auto EditorWorkspace::requestCloseSaveAs() -> void {
        m_closeSaveAs = true;
    }

    auto EditorWorkspace::requestCloseLoad() -> void {
        m_closeLoad = true;
    }

    auto EditorWorkspace::requestConfirmLoad() -> void {
        m_openLoadConfirmation = true;
    }

    auto EditorWorkspace::setAvailableScenes(std::vector<std::string> scenes) -> void {
        m_availableScenes = std::move(scenes);
    }

    auto EditorWorkspace::hasPendingChanges(AetherEngine::scene::Scene &scene) const -> bool {
        return m_inspector->hasPendingChanges(scene);
    }

    auto EditorWorkspace::drawDockspace() -> void {
        ImGui::DockSpaceOverViewport(
            0,
            ImGui::GetMainViewport(),
            ImGuiDockNodeFlags_PassthruCentralNode
        );
    }

    auto EditorWorkspace::drawPanels(AetherEngine::components::CameraComponent &camera, AetherEngine::scene::Scene &scene, core::EditorSession &session, commands
                                     ::CommandHistory &history, const AetherEngine::logging::Logger &logger, const AetherEngine::assets::AssetRegistry &
                                     registry) -> EditorWorkspaceFrame {
        EditorWorkspaceFrame frame{};

        if (m_showHierarchy) {
            auto hierarchy = m_hierarchy->draw(m_showHierarchy, scene, session);
            frame.createEntityRequested = hierarchy.createEntityRequested;
            frame.renameEntityRequested = std::move(hierarchy.renameEntityRequested);
        } else {
            m_hierarchy->cancelRename();
        }
        if (m_showInspector) {
            frame.sceneChanged =
                m_inspector->draw(m_showInspector, scene, session, history, registry);
        } else {
            m_inspector->finishTransformEdit(scene, history);
        }
        if (m_showSceneViewport) {
            frame.viewport = m_sceneViewport->draw(m_showSceneViewport, camera);
        }
        if (m_showConsole) {
            m_console->draw(m_showConsole, logger);
        }
        if (m_showAssetBrowser) {
            frame.assets = m_assetBrowser->draw(m_showAssetBrowser, registry.listAssets());
        }
        return frame;
    }

    auto EditorWorkspace::drawMenuBar(const commands::CommandHistory &history, EditorWorkspaceFrame &frame, const docs::SceneEditorDocument &document) -> void {
        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                frame.saveRequested = ImGui::MenuItem(
                    "Save",
                    "Ctrl+S",
                    false,
                    document.scene != nullptr
                );
                if (ImGui::MenuItem("Save As...")) {
                    m_openSaveAs = true;
                }
                if (ImGui::MenuItem("New Scene")) {
                    frame.documentActionRequested = core::PendingDocumentAction{
                        .kind = core::DocumentActionKind::NewScene,
                        .sceneName = "Untitled"
                    };
                }
                if (ImGui::MenuItem("Open...")) {
                    m_openLoad = true;
                    frame.refreshSceneListRequested = true;
                }
                ImGui::EndMenu();
            }
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
            const char* sceneName = document.fileStem.empty() ? "Untitled" : document.fileStem.c_str();

            ImGui::SameLine();
            ImGui::Text(
                "%s%s",
                sceneName,
                document.dirty ? " *" : ""
            );
            ImGui::EndMainMenuBar();
            if (m_openSaveAs) {
                ImGui::OpenPopup("Save scene as");
                m_openSaveAs = false;
            }

            if (ImGui::BeginPopupModal(
                    "Save scene as",
                    nullptr,
                    ImGuiWindowFlags_AlwaysAutoResize
                )) {
                if (m_closeSaveAs) {
                    m_closeSaveAs = false;
                    ImGui::CloseCurrentPopup();
                } else {
                    ImGui::InputText(
                        "Scene name",
                        m_saveAsName,
                        sizeof(m_saveAsName)
                    );

                    if (ImGui::Button("Cancel")) {
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::BeginDisabled(m_saveAsName[0] == '\0');

                    if (ImGui::Button("Save")) {
                        frame.saveAsFileStem = std::string{m_saveAsName};
                    }

                    ImGui::EndDisabled();
                }
                ImGui::EndPopup();
            }
            if (m_openLoad) {
                m_closeLoad = false;
                ImGui::OpenPopup("Open scene");
                m_openLoad = false;
            }
            if (ImGui::BeginPopupModal(
                "Open scene",
                nullptr,
                ImGuiWindowFlags_AlwaysAutoResize)) {
                if (m_closeLoad) {
                    ImGui::CloseCurrentPopup();
                } else {
                    ImGui::InputText("Scene name", &m_loadName);

                    if (ImGui::BeginListBox("Scenes", ImVec2{360, 200})) {
                        for (const auto& name : m_availableScenes) {
                            const bool selected = m_loadName == name;

                            if (ImGui::Selectable(name.c_str(), selected)) {
                                m_loadName = name;
                            }
                        }

                        ImGui::EndListBox();
                    }

                    if (ImGui::Button("Refresh")) {
                        frame.refreshSceneListRequested = true;
                    }

                    ImGui::BeginDisabled(m_loadName.empty());

                    if (ImGui::Button("Open")) {
                        frame.documentActionRequested = core::PendingDocumentAction{
                            core::DocumentActionKind::OpenScene,
                            m_loadName
                        };
                    }

                    ImGui::EndDisabled();

                    ImGui::SameLine();

                    if (ImGui::Button("Cancel")) {
                        ImGui::CloseCurrentPopup();
                    }
                }
                ImGui::EndPopup();
            }
            m_closeLoad = false;
            if (m_openLoadConfirmation) {
                ImGui::OpenPopup("Unsaved changes");
                m_openLoadConfirmation = false;
            }

            if (ImGui::BeginPopupModal(
                "Unsaved changes",
                nullptr,
                ImGuiWindowFlags_AlwaysAutoResize
            )) {
                ImGui::TextUnformatted("The current scene has unsaved changes.");
                ImGui::TextUnformatted("Open another scene without saving?");

                if (ImGui::Button("Open without saving")) {
                    frame.loadDecision = core::SceneLoadDecision::DiscardChanges;
                    ImGui::CloseCurrentPopup();
                }

                ImGui::SameLine();

                if (ImGui::Button("Cancel")) {
                    frame.loadDecision = core::SceneLoadDecision::Cancel;
                    ImGui::CloseCurrentPopup();
                }

                ImGui::EndPopup();
            }
        }
    }

    auto EditorWorkspace::processHistoryShortcuts(const commands::CommandHistory &history,
                                                  EditorWorkspaceFrame &frame, const docs::SceneEditorDocument &document) const -> void {
        constexpr ImGuiInputFlags flags = ImGuiInputFlags_RouteGlobal;

        const bool undoPressed = ImGui::Shortcut(ImGuiMod_Ctrl  | ImGuiKey_Z, flags);
        const bool redoPressed = ImGui::Shortcut(ImGuiMod_Ctrl  | ImGuiKey_Y, flags);

        const bool redoShiftZPressed = ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_Y, flags);
        const bool savePressed = ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_S, flags);
        if (ImGui::IsPopupOpen(
            nullptr,
            ImGuiPopupFlags_AnyPopupId | ImGuiPopupFlags_AnyPopupLevel
        )) {
            return;
        }
        if (frame.undoRequested || frame.redoRequested || frame.saveRequested) {
            return;
        }
        if (ImGui::IsAnyItemActive() || m_inspector->hasPendingEdit() || m_hierarchy->isRenaming()) {
            return;
        }
        if (savePressed && document.scene != nullptr) {
            frame.saveRequested = true;
        } else if (undoPressed && history.canUndo()) {
            frame.undoRequested = true;
        } else if ((redoPressed || redoShiftZPressed) && history.canRedo()) {
            frame.redoRequested = true;
        }
    }
} // ui
// AetherEditor
