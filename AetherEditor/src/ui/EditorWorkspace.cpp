//
// Created by drhaz on 08.10.2026.
//

#include "ui/EditorWorkspace.hpp"
#include "security/PathSecurity.hpp"


namespace AetherEditor::ui {
    auto EditorWorkspace::create() -> EditorWorkspacePtr {
        return EditorWorkspacePtr(new EditorWorkspace(), EditorWorkspaceDeleter{});
    }

    auto EditorWorkspace::draw(AetherEngine::components::CameraComponent &camera, AetherEngine::scene::Scene &scene, AetherEditor::core::EditorSession& session, commands
                               ::CommandHistory &history, const docs::SceneEditorDocument &document, const AetherEngine::logging::Logger &logger, const
                               AetherEngine::assets::AssetRegistry &registry,
                               const AetherEngine::systems::SceneAssetBindingSystem& bindings,
                               bool simulationRunning, bool documentActionPending) -> EditorWorkspaceFrame {
        EditorWorkspaceFrame frame{};

        drawMenuBar(history, frame, document, simulationRunning, documentActionPending);
        drawDocumentDialogs(frame, document);
        drawDockspace();

        auto panels = drawPanels(camera, scene, session, history, logger, registry, bindings,
            simulationRunning || documentActionPending);
        frame.viewport = panels.viewport;
        frame.sceneChanged = panels.sceneChanged;
        frame.hierarchyFocused = panels.hierarchyFocused;
        frame.createEntityRequested = panels.createEntityRequested;
        frame.renameEntityRequested = std::move(panels.renameEntityRequested);
        frame.deleteEntityRequested = panels.deleteEntityRequested;
        frame.duplicateEntityRequested = panels.duplicateEntityRequested;
        frame.assets = std::move(panels.assets);
        if (!simulationRunning && !documentActionPending && frame.simulationAction == SimulationAction::None) {
            processHistoryShortcuts(history, frame, document);
            processEntityShortcuts(session, frame);
        }
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
        m_inspector->finishPendingEdits(scene, history);
    }

    auto EditorWorkspace::requestOpenSaveAs() -> void {
        m_openSaveAs = true;
        m_closeSaveAs = false;
        m_saveAsError.clear();
        m_overwriteTarget.reset();
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

    auto EditorWorkspace::requestConfirmLoad(core::DocumentActionKind kind) -> void {
        m_confirmAction = kind;
        m_openLoadConfirmation = true;
    }

    auto EditorWorkspace::requestOverwrite(std::string filename) -> void {
        m_overwriteTarget = std::move(filename);
        m_openOverwrite = true;
        m_saveAsError.clear();
    }

    auto EditorWorkspace::setSaveAsError(std::string message) -> void {
        m_saveAsError = std::move(message);
    }

    auto EditorWorkspace::hasOpenPopup() const -> bool {
        return ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopupId | ImGuiPopupFlags_AnyPopupLevel);
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
                                     registry, const AetherEngine::systems::SceneAssetBindingSystem& bindings,
                                     bool simulationRunning) -> EditorWorkspaceFrame {
        EditorWorkspaceFrame frame{};

        if (m_showHierarchy) {
            auto hierarchy = m_hierarchy->draw(m_showHierarchy, scene, session, simulationRunning);
            frame.hierarchyFocused = hierarchy.focused;
            frame.createEntityRequested = hierarchy.createEntityRequested;
            frame.renameEntityRequested = std::move(hierarchy.renameEntityRequested);
            frame.deleteEntityRequested = hierarchy.deleteEntityRequested;
            frame.duplicateEntityRequested = hierarchy.duplicateEntityRequested;
        } else {
            m_hierarchy->cancelRename();
        }
        if (m_showInspector) {
            ImGui::BeginDisabled(simulationRunning);
            frame.sceneChanged =
                m_inspector->draw(m_showInspector, scene, session, history, registry, bindings.errors());
            ImGui::EndDisabled();
        } else {
            m_inspector->finishPendingEdits(scene, history);
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

    auto EditorWorkspace::drawMenuBar(const commands::CommandHistory &history, EditorWorkspaceFrame &frame, const docs::SceneEditorDocument &document,
        bool simulationRunning, bool documentActionPending) -> void {
        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                ImGui::BeginDisabled(simulationRunning || documentActionPending);
                frame.saveRequested = ImGui::MenuItem(
                    "Save",
                    "Ctrl+S",
                    false,
                    document.scene != nullptr
                );
                if (ImGui::MenuItem("Save As...")) {
                    requestOpenSaveAs();
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
                ImGui::EndDisabled();
                ImGui::Separator();
                if (ImGui::MenuItem("Exit")) {
                    frame.documentActionRequested = core::PendingDocumentAction{core::DocumentActionKind::Quit, {}};
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("View")) {
                ImGui::MenuItem("Asset Browser", nullptr, &m_showAssetBrowser);
                ImGui::MenuItem("Console", nullptr, &m_showConsole);
                ImGui::MenuItem("Hierarchy", nullptr, &m_showHierarchy);
                ImGui::MenuItem("Inspector", nullptr, &m_showInspector);
                ImGui::MenuItem("Scene Viewport", nullptr, &m_showSceneViewport);
                ImGui::Separator();
                frame.retryAssetsRequested = ImGui::MenuItem("Retry Missing Assets");

                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Edit", !simulationRunning && !documentActionPending)) {
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
            const bool popupOpen = ImGui::IsPopupOpen(
                nullptr, ImGuiPopupFlags_AnyPopupId | ImGuiPopupFlags_AnyPopupLevel
            );
            ImGui::BeginDisabled((!simulationRunning && document.scene == nullptr) || popupOpen || documentActionPending);
            ImGui::PushStyleColor(ImGuiCol_Button, simulationRunning
                ? ImVec4{0.60f, 0.20f, 0.20f, 1.0f} : ImVec4{0.16f, 0.42f, 0.25f, 1.0f});
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, simulationRunning
                ? ImVec4{0.75f, 0.26f, 0.26f, 1.0f} : ImVec4{0.20f, 0.54f, 0.32f, 1.0f});
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, simulationRunning
                ? ImVec4{0.48f, 0.16f, 0.16f, 1.0f} : ImVec4{0.12f, 0.34f, 0.20f, 1.0f});
            if (ImGui::Button(simulationRunning ? "Stop###SimulationToggle" : "Play###SimulationToggle",
                              ImVec2{ImGui::GetFontSize() * 5.0f, 0.0f})) {
                frame.simulationAction = simulationRunning ? SimulationAction::Stop : SimulationAction::Run;
            }
            ImGui::PopStyleColor(3);
            ImGui::EndDisabled();
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("%s", simulationRunning
                    ? "Stop simulation and restore the scene before Play."
                    : "Simulate a copy of the current scene, including unsaved edits.");
            }

            const char* sceneName = document.fileStem.empty() ? "Untitled" : document.fileStem.c_str();

            ImGui::SameLine();
            ImGui::Text(
                "%s%s",
                sceneName,
                document.dirty ? " *" : ""
            );
            ImGui::EndMainMenuBar();
        }
    }

    auto EditorWorkspace::drawDocumentDialogs(EditorWorkspaceFrame& frame, const docs::SceneEditorDocument& document) -> void {
        if (m_openSaveAs) {
            m_saveAsName = document.fileStem;
            ImGui::OpenPopup("Save scene as");
            m_openSaveAs = false;
        }
        if (ImGui::BeginPopupModal("Save scene as", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            if (m_closeSaveAs) {
                m_closeSaveAs = false;
                m_overwriteTarget.reset();
                ImGui::CloseCurrentPopup();
            } else {
                ImGui::TextUnformatted("Save in the project's assets/scenes folder.");
                if (ImGui::InputText("Scene name", &m_saveAsName)) m_saveAsError.clear();
                ImGui::TextDisabled("Enter a name without the .scene.json extension.");
                if (!m_saveAsError.empty()) {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{1.0f, 0.45f, 0.4f, 1.0f});
                    ImGui::TextWrapped("%s", m_saveAsError.c_str());
                    ImGui::PopStyleColor();
                }
                const bool validName = AetherEngine::security::isSafeSceneName(m_saveAsName);
                if (!m_saveAsName.empty() && !validName) ImGui::TextUnformatted("Use a valid file name (up to 128 bytes, no path separators).");
                ImGui::BeginDisabled(!validName);
                if (ImGui::Button("Save")) {
                    frame.saveAsFileStem = m_saveAsName;
                    m_saveAsError.clear();
                }
                ImGui::EndDisabled();
                ImGui::SameLine();
                if (ImGui::Button("Cancel") || (!m_overwriteTarget && ImGui::IsKeyPressed(ImGuiKey_Escape))) {
                    frame.saveAsCancelled = true;
                    ImGui::CloseCurrentPopup();
                }
                // This confirmation belongs to the Save As modal, so the exact
                // name awaiting approval cannot be edited behind it.
                if (m_openOverwrite) {
                    ImGui::OpenPopup("Replace scene?");
                    m_openOverwrite = false;
                }
                if (ImGui::BeginPopupModal("Replace scene?", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
                    ImGui::TextWrapped("The scene '%s.scene.json' already exists. Replace it?",
                        m_overwriteTarget ? m_overwriteTarget->c_str() : "");
                    if (ImGui::Button("Replace") && m_overwriteTarget) {
                        frame.saveAsFileStem = *m_overwriteTarget;
                        frame.overwriteConfirmed = true;
                        m_overwriteTarget.reset();
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Cancel") || ImGui::IsKeyPressed(ImGuiKey_Escape)) {
                        m_overwriteTarget.reset();
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::EndPopup();
                }
            }
            ImGui::EndPopup();
        }

        if (m_openLoad) {
            m_closeLoad = false;
            ImGui::OpenPopup("Open scene");
            m_openLoad = false;
        }
        if (ImGui::BeginPopupModal("Open scene", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            if (m_closeLoad) {
                ImGui::CloseCurrentPopup();
            } else {
                ImGui::InputText("Scene name", &m_loadName);
                if (ImGui::BeginListBox("Scenes", ImVec2{360, 200})) {
                    for (const auto& name : m_availableScenes) {
                        if (ImGui::Selectable(name.c_str(), m_loadName == name)) m_loadName = name;
                    }
                    ImGui::EndListBox();
                }
                if (ImGui::Button("Refresh")) frame.refreshSceneListRequested = true;
                ImGui::SameLine();
                ImGui::BeginDisabled(m_loadName.empty());
                if (ImGui::Button("Open")) {
                    frame.documentActionRequested = core::PendingDocumentAction{core::DocumentActionKind::OpenScene, m_loadName};
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndDisabled();
                ImGui::SameLine();
                if (ImGui::Button("Cancel") || ImGui::IsKeyPressed(ImGuiKey_Escape)) ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
        m_closeLoad = false;

        if (m_openLoadConfirmation) {
            ImGui::OpenPopup("Unsaved changes");
            m_openLoadConfirmation = false;
        }
        if (ImGui::BeginPopupModal("Unsaved changes", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::TextUnformatted(m_confirmAction == core::DocumentActionKind::Quit
                ? "Save changes to the authoring scene before exiting?"
                : "Save changes to the current scene before continuing?");
            if (ImGui::Button("Save")) {
                frame.loadDecision = core::SceneLoadDecision::SaveChanges;
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Discard")) {
                frame.loadDecision = core::SceneLoadDecision::DiscardChanges;
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel") || ImGui::IsKeyPressed(ImGuiKey_Escape)) {
                frame.loadDecision = core::SceneLoadDecision::Cancel;
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
    }
    auto EditorWorkspace::processEntityShortcuts(const core::EditorSession& session, EditorWorkspaceFrame& frame) const -> void {
        constexpr ImGuiInputFlags flags = ImGuiInputFlags_RouteGlobal;
        const bool deletePressed = ImGui::Shortcut(ImGuiKey_Delete, flags);
        const bool duplicatePressed = ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_D, flags);
        if (!session.selectedEntity || (!frame.hierarchyFocused && !frame.viewport.state.focused) ||
            ImGui::IsAnyItemActive() || ImGui::GetIO().WantTextInput ||
            m_inspector->hasPendingEdit() || m_hierarchy->isRenaming() ||
            ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopupId | ImGuiPopupFlags_AnyPopupLevel) ||
            frame.createEntityRequested || frame.renameEntityRequested || frame.deleteEntityRequested ||
            frame.duplicateEntityRequested || frame.undoRequested || frame.redoRequested || frame.saveRequested ||
            frame.saveAsFileStem || frame.documentActionRequested) {
            return;
        }
        if (deletePressed) {
            frame.deleteEntityRequested = session.selectedEntity;
        } else if (duplicatePressed) {
            frame.duplicateEntityRequested = session.selectedEntity;
        }
    }

    auto EditorWorkspace::processHistoryShortcuts(const commands::CommandHistory &history,
                                                  EditorWorkspaceFrame &frame, const docs::SceneEditorDocument &document) const -> void {
        constexpr ImGuiInputFlags flags = ImGuiInputFlags_RouteGlobal;

        const bool undoPressed = ImGui::Shortcut(ImGuiMod_Ctrl  | ImGuiKey_Z, flags);
        const bool redoPressed = ImGui::Shortcut(ImGuiMod_Ctrl  | ImGuiKey_Y, flags);

        const bool redoShiftZPressed = ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_Z, flags);
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
