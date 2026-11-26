#include "SceneWriteTestSupport.hpp"
#include "ui/EditorWorkspace.hpp"
#include "components/PhysicsComponents.hpp"
#include "components/TransformComponent.hpp"
#include "systems/SceneSerializer.hpp"
#include <imgui_internal.h>

namespace {
    using namespace scene_write_tests;
    namespace ae = AetherEngine;
    using Decision = AetherEditor::core::SceneLoadDecision;
    using Action = AetherEditor::core::DocumentActionKind;

    struct UiFixture {
        Fixture files;
        ae::logging::Logger logger;
        ae::resources::ResourceManager::ResourceManagerPtr resources = ae::resources::ResourceManager::createResourceManager(files.root / "assets");
        std::unique_ptr<ae::assets::AssetManager> assets;
        ae::systems::SceneAssetBindingSystem::SceneAssetBindingSystemPtr bindings;
        ae::scene::Scene::ScenePtr scene = ae::scene::Scene::createScene("Scene");
        AetherEditor::ui::EditorWorkspace::EditorWorkspacePtr workspace = AetherEditor::ui::EditorWorkspace::create();
        AetherEditor::core::EditorSession session;
        AetherEditor::commands::CommandHistory history;
        AetherEditor::docs::SceneEditorDocument document;
        ae::components::CameraComponent camera;

        UiFixture() {
            fs::create_directories(files.root / "assets/registry");
            put(files.root / "assets/registry/runtime-assets.json", R"({"version":1,"assets":[]})");
            auto created = ae::assets::AssetManager::create(files.root / "assets", *resources,
                {ae::assets::OperatingSystem::Windows, ae::assets::Architecture::X64, ae::assets::GraphicsBackend::Direct3D11});
            require(created.has_value(), "Could not create UI fixture assets");
            assets = std::move(*created);
            bindings = ae::systems::SceneAssetBindingSystem::create(*assets);
            ImGui::CreateContext();
            auto& io = ImGui::GetIO();
            io.IniFilename = nullptr;
            io.DisplaySize = {1280, 800};
            io.DeltaTime = 1.0f / 60.0f;
            io.ConfigFlags |= ImGuiConfigFlags_DockingEnable | ImGuiConfigFlags_NavEnableKeyboard;
            unsigned char* pixels;
            int width, height;
            io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
            io.Fonts->SetTexID(1);
            require(workspace->init().has_value(), "Workspace initialization failed");
            using Panel = AetherEditor::core::EditorWorkspacePanels;
            for (const auto panel : {Panel::Hierarchy, Panel::Inspector, Panel::SceneViewport, Panel::Console, Panel::AssetBrowser}) {
                require(workspace->setPanelVisible(panel, false).has_value(), "Could not hide a panel");
            }
            document.scene = scene.get();
            document.fileStem = "Scene";
            document.dirty = true;
            frame();
        }
        ~UiFixture() {
            workspace.reset();
            ImGui::DestroyContext();
        }
        auto frame(bool simulation = false, bool pending = true) -> AetherEditor::ui::EditorWorkspaceFrame {
            ImGui::NewFrame();
            auto result = workspace->draw(camera, *scene, session, history, document, logger, assets->registry(), *bindings, simulation, pending);
            ImGui::Render();
            return result;
        }
        auto press(const char* windowName, const char* button, bool simulation = false) -> AetherEditor::ui::EditorWorkspaceFrame {
            auto* window = ImGui::FindWindowByName(windowName);
            require(window != nullptr && window->Active, "Expected dialog is not active");
            ImGui::ActivateItemByID(window->GetID(button));
            return frame(simulation);
        }
        auto topIs(const char* name) -> bool {
            const auto* window = ImGui::GetTopMostPopupModal();
            return window && std::string_view{window->Name} == name;
        }
        auto showPhysics() -> ae::scene::Entity {
            const auto entity = scene->createEntity("Physics object");
            scene->addComponent<ae::components::TransformComponent>(entity);
            scene->addComponent<ae::components::RigidbodyComponent>(entity);
            scene->addComponent<ae::components::ColliderComponent>(entity);
            session.selectedEntity = scene->getComponent<ae::components::IdentityComponent>(entity).id;
            require(workspace->setPanelVisible(AetherEditor::core::EditorWorkspacePanels::Inspector, true).has_value(), "Inspector hidden");
            frame(false, false);
            auto* window = ImGui::FindWindowByName("Inspector");
            ImGui::SetWindowPos(window, {20, 40}, ImGuiCond_Always);
            ImGui::SetWindowSize(window, {600, 740}, ImGuiCond_Always);
            frame(false, false);
            frame(false, false);
            return entity;
        }
        auto inspectorId(const char* section, const char* label) -> ImGuiID {
            auto* window = ImGui::FindWindowByName("Inspector");
            const auto entity = ae::scene::toString(*session.selectedEntity);
            const auto entityId = ImHashStr(entity.c_str(), 0, window->ID);
            const auto sectionId = ImHashStr(section, 0, entityId);
            return ImHashStr(label, 0, sectionId);
        }
        auto activate(const char* section, const char* label, bool simulation = false) -> void {
            ImGui::ActivateItemByID(inspectorId(section, label));
            frame(simulation, false);
        }
        auto key(ImGuiKey key) -> void {
            ImGui::GetIO().AddKeyEvent(key, true);
            frame(false, false);
            ImGui::GetIO().AddKeyEvent(key, false);
            frame(false, false);
        }
        auto beginNumber(const char* section, const char* label, const char* text) -> void {
            beginNumber(inspectorId(section, label), text);
        }
        auto beginNumber(ImGuiID id, const char* text) -> void {
            ImGui::ActivateItemByID(id);
            GImGui->NavNextActivateFlags = ImGuiActivateFlags_PreferInput;
            frame(false, false);
            require(ImGui::GetInputTextState(id) != nullptr, "Numeric field did not enter text editing");
            ImGui::GetIO().AddInputCharactersUTF8(text);
            frame(false, false);
        }
    };

    void physicsInspectorHistory() {
        UiFixture f;
        const auto entity = f.showPhysics();
        auto& body = f.scene->getComponent<ae::components::RigidbodyComponent>(entity);
        auto& collider = f.scene->getComponent<ae::components::ColliderComponent>(entity);
        collider.type = ae::components::ColliderType::Capsule;
        collider.trigger = true;
        collider.height = 2.0f;
        f.frame(false, false);

        f.beginNumber("Rigidbody", "Mass", "2");
        require(body.mass == 2.0f && f.workspace->hasPendingChanges(*f.scene) && !f.history.canUndo(),
            "Mass input did not update the component as a pending edit");
        ImGui::GetIO().AddInputCharactersUTF8("5");
        f.frame(false, false);
        require(body.mass == 25.0f, "Continued mass edit was not applied");
        f.key(ImGuiKey_Enter);
        require(!f.workspace->hasPendingChanges(*f.scene) && f.history.canUndo(), "Mass edit was not committed");
        require(f.history.undo(*f.scene) && body.mass == 1.0f && !f.history.canUndo() && collider.trigger && collider.height == 2.0f,
            "One numeric edit was not one undo or reset the Collider");
        require(f.history.redo(*f.scene) && body.mass == 25.0f, "Mass redo failed");
        f.history.clear();

        f.activate("Rigidbody", "Use Gravity");
        require(!body.useGravity && f.history.undo(*f.scene) && body.useGravity && !f.history.canUndo(), "Gravity checkbox history failed");
        f.history.clear();
        f.activate("Collider", "Sensor / Trigger");
        require(!collider.trigger && f.history.undo(*f.scene) && collider.trigger, "Trigger checkbox history failed");
        f.history.clear();

        f.activate("Collider", "Collision Enabled");
        require(!collider.collisionEnabled && collider.trigger && body.useGravity, "Disabling collisions changed sensor or gravity");
        f.activate("Collider", "Sensor / Trigger");
        require(collider.trigger, "Disabled sensor checkbox accepted input");
        require(f.history.undo(*f.scene) && collider.collisionEnabled && !f.history.canUndo(), "Collision Enabled undo failed");
        require(f.history.redo(*f.scene) && !collider.collisionEnabled, "Collision Enabled redo failed");
        f.history.clear();

        f.beginNumber("Collider", "Radius", "0.75");
        f.key(ImGuiKey_Enter);
        require(collider.radius == 0.75f && collider.height == 2.0f && body.mass == 25.0f, "Radius edit changed unrelated fields");
        require(f.history.undo(*f.scene) && collider.radius == 0.5f && !f.history.canUndo(), "Radius undo failed");
        f.history.clear();
        f.beginNumber("Collider", "Cylinder Height", "3.5");
        f.key(ImGuiKey_Enter);
        require(collider.height == 3.5f && f.history.undo(*f.scene) && collider.height == 2.0f, "Cylinder height edit/undo failed");
    }

    void physicsInspectorLifecycle() {
        UiFixture f;
        const auto entity = f.showPhysics();
        auto& body = f.scene->getComponent<ae::components::RigidbodyComponent>(entity);
        f.beginNumber("Rigidbody", "Mass", "4");
        f.workspace->finishPendingEdits(*f.scene, f.history);
        auto serializer = ae::systems::SceneSerializer::createSceneSerializer(f.files.scenes);
        require(serializer->serializeScene(*f.scene).has_value(), "Save after finishing physics input failed");
        f.history.markSaved();
        f.key(ImGuiKey_Enter);
        require(f.history.undo(*f.scene) && body.mass == 1.0f && f.history.isDirty(), "Save did not retain physics undo");
        require(f.history.redo(*f.scene) && body.mass == 4.0f && !f.history.isDirty(), "Saved physics redo was not clean");
        f.history.clear();

        f.beginNumber("Rigidbody", "Mass", "6");
        f.session.selectedEntity.reset();
        f.frame(false, false);
        require(f.history.undo(*f.scene) && body.mass == 4.0f, "Selection change lost the pending physics edit");
        f.session.selectedEntity = f.scene->getComponent<ae::components::IdentityComponent>(entity).id;
        f.frame(false, false);
        f.history.clear();
        f.beginNumber("Rigidbody", "Mass", "8");
        require(f.workspace->setPanelVisible(AetherEditor::core::EditorWorkspacePanels::Inspector, false).has_value(), "Could not hide Inspector");
        f.frame(false, false);
        require(f.history.undo(*f.scene) && body.mass == 4.0f, "Closing Inspector lost the pending physics edit");
        f.history.clear();
        require(f.workspace->setPanelVisible(AetherEditor::core::EditorWorkspacePanels::Inspector, true).has_value(), "Could not show Inspector");
        f.frame(false, false);
        f.beginNumber("Rigidbody", "Mass", "9");
        f.key(ImGuiKey_Escape);
        require(body.mass == 4.0f && !f.history.canUndo(), "Escape did not cancel the pending physics edit");
        f.activate("Rigidbody", "Use Gravity", true);
        require(body.useGravity && !f.history.canUndo(), "Inspector edited physics during Play");
    }

    void physicsInspectorAddRemove() {
        UiFixture f;
        const auto entity = f.showPhysics();
        f.scene->removeComponent<ae::components::RigidbodyComponent, ae::components::ColliderComponent>(entity);
        f.frame(false, false);
        f.activate("Physics", "Physics");
        require(f.scene->hasComponent<ae::components::RigidbodyComponent, ae::components::ColliderComponent>(entity) &&
                !f.scene->getComponent<ae::components::RigidbodyComponent>(entity).dynamic, "Add Physics did not create a Static body and collider");
        require(f.history.undo(*f.scene) && !f.scene->hasComponent<ae::components::ColliderComponent>(entity), "Add Physics undo failed");
        require(f.history.redo(*f.scene), "Add Physics redo failed");
        f.scene->getComponent<ae::components::ColliderComponent>(entity).trigger = true;
        f.history.clear();
        f.activate("Physics", "Physics");
        require(!f.scene->hasComponent<ae::components::RigidbodyComponent>(entity) && !f.scene->hasComponent<ae::components::ColliderComponent>(entity),
            "Remove Physics left components");
        require(f.history.undo(*f.scene) && f.scene->getComponent<ae::components::ColliderComponent>(entity).trigger,
            "Remove Physics undo lost sensor state");
        f.history.clear();
        f.activate("Physics", "Physics", true);
        require(f.scene->hasComponent<ae::components::RigidbodyComponent, ae::components::ColliderComponent>(entity) && !f.history.canUndo(),
            "Physics checkbox removed components during Play");

        f.scene->removeComponent<ae::components::RigidbodyComponent>(entity);
        f.frame(false, false);
        f.activate("Physics", "Physics");
        require(f.scene->hasComponent<ae::components::RigidbodyComponent>(entity) &&
                f.scene->getComponent<ae::components::ColliderComponent>(entity).trigger,
            "Physics checkbox failed to complete a partial set while preserving settings");
        require(f.history.undo(*f.scene) && !f.scene->hasComponent<ae::components::RigidbodyComponent>(entity) &&
                f.scene->getComponent<ae::components::ColliderComponent>(entity).trigger,
            "Undo completion did not restore the incomplete set");
        f.activate("Physics", "Remove incomplete physics");
        require(!f.scene->hasComponent<ae::components::ColliderComponent>(entity), "Incomplete physics could not be removed");
        require(f.history.undo(*f.scene) && f.scene->getComponent<ae::components::ColliderComponent>(entity).trigger,
            "Undo incomplete removal lost the original component");
    }

    void physicsInspectorShapesAndDrag() {
        UiFixture f;
        const auto entity = f.showPhysics();
        auto& body = f.scene->getComponent<ae::components::RigidbodyComponent>(entity);
        auto& collider = f.scene->getComponent<ae::components::ColliderComponent>(entity);
        const auto dimensionGroup = f.inspectorId("Collider", "Dimensions");
        const int xAxis = 0;
        const auto xField = ImHashStr("", 0, ImHashData(&xAxis, sizeof(xAxis), dimensionGroup));
        f.beginNumber(xField, "6");
        f.key(ImGuiKey_Enter);
        require(collider.size == ae::math::Vec3{3, 1, 1}, "Box full dimensions were not converted to half extents");
        require(f.history.undo(*f.scene) && collider.size == ae::math::Vec3::one() && !f.history.canUndo(), "Box dimensions undo failed");
        f.history.clear();

        f.activate("Collider", "Shape");
        f.key(ImGuiKey_DownArrow);
        f.key(ImGuiKey_Enter);
        require(collider.type == ae::components::ColliderType::Sphere && f.history.undo(*f.scene) && collider.type == ae::components::ColliderType::Box,
            "Shape selection was not undoable");
        f.history.clear();
        f.activate("Rigidbody", "Body Type");
        f.key(ImGuiKey_UpArrow);
        f.key(ImGuiKey_Enter);
        require(!body.dynamic && f.history.canUndo(), "Static body selection failed");
        f.activate("Rigidbody", "Use Gravity");
        require(body.useGravity, "Gravity checkbox remained editable for Static");
        require(f.history.undo(*f.scene) && body.dynamic && !f.history.canUndo(), "Body Type undo failed");
        f.history.clear();

        // Locate the focused field through ImGui's navigation rectangle, then
        // exercise real mouse input across frames (no direct component writes).
        f.activate("Rigidbody", "Mass");
        f.frame(false, false);
        auto* window = ImGui::FindWindowByName("Inspector");
        const auto rect = ImGui::WindowRectRelToAbs(window, window->NavRectRel[ImGuiNavLayer_Main]);
        const float x = (rect.Min.x + rect.Max.x) * 0.5f;
        const float y = (rect.Min.y + rect.Max.y) * 0.5f;
        f.key(ImGuiKey_Escape);
        auto& io = ImGui::GetIO();
        io.AddMousePosEvent(x, y);
        f.frame(false, false);
        io.AddMouseButtonEvent(0, true);
        f.frame(false, false);
        io.AddMousePosEvent(x + 30, y);
        f.frame(false, false);
        io.AddMousePosEvent(x + 60, y);
        f.frame(false, false);
        require(body.mass > 1.0f && f.workspace->hasPendingChanges(*f.scene) && !f.history.canUndo(), "Mouse drag was not a pending physics edit");
        const float draggedMass = body.mass;
        io.AddMouseButtonEvent(0, false);
        f.frame(false, false);
        require(f.history.undo(*f.scene) && body.mass == 1.0f && !f.history.canUndo(), "Mouse drag generated more than one undo command");
        require(f.history.redo(*f.scene) && body.mass == draggedMass, "Mouse drag redo failed");
    }

    void redoKeyboardShortcuts() {
        UiFixture f;
        f.showPhysics();
        f.activate("Rigidbody", "Use Gravity");
        require(f.history.undo(*f.scene) && f.history.canRedo(), "Could not prepare redo history");
        ImGui::ClearActiveID();
        auto& io = ImGui::GetIO();
        io.AddKeyEvent(ImGuiMod_Ctrl, true);
        io.AddKeyEvent(ImGuiMod_Shift, true);
        f.frame(false, false);
        io.AddKeyEvent(ImGuiKey_Z, true);
        auto result = f.frame(false, false);
        require(result.redoRequested && !result.undoRequested, "Ctrl+Shift+Z did not request redo");
        io.AddKeyEvent(ImGuiKey_Z, false);
        io.AddKeyEvent(ImGuiMod_Shift, false);
        f.frame(false, false);
        io.AddKeyEvent(ImGuiKey_Y, true);
        result = f.frame(false, false);
        require(result.redoRequested && !result.undoRequested, "Ctrl+Y did not request redo");
    }

    void saveAndOverwriteDialogs() {
        UiFixture f;
        f.workspace->requestConfirmLoad(Action::Quit);
        f.frame(true);
        require(f.topIs("Unsaved changes"), "Quit did not open confirmation during Play");
        const auto save = f.press("Unsaved changes", "Save", true);
        require(save.loadDecision == Decision::SaveChanges, "Save button did not request saving");
        f.workspace->requestOpenSaveAs();
        f.frame(true);
        require(f.topIs("Save scene as"), "Save As did not replace the confirmation");
        const auto create = f.press("Save scene as", "Save", true);
        require(create.saveAsFileStem == "Scene" && !create.overwriteConfirmed, "Initial Save As authorized an overwrite");
        f.workspace->requestOverwrite("Existing");
        f.frame(true);
        require(f.topIs("Replace scene?"), "Overwrite confirmation did not open above Save As");
        const auto cancel = f.press("Replace scene?", "Cancel", true);
        require(!cancel.saveAsFileStem && !cancel.saveAsCancelled, "Cancelling overwrite saved or cancelled the whole workflow");
        f.frame(true);
        require(f.topIs("Save scene as"), "Cancelling overwrite closed Save As");
        f.workspace->requestOverwrite("Existing");
        f.frame(true);
        const auto replace = f.press("Replace scene?", "Replace", true);
        require(replace.overwriteConfirmed && replace.saveAsFileStem == "Existing", "Overwrite did not approve the exact pending filename");
        f.workspace->requestCloseSaveAs();
        f.frame(true);
        require(!f.workspace->hasOpenPopup(), "Successful Save As left a blocking dialog");
    }

    void cancelAndDiscardDialogs() {
        UiFixture f;
        f.workspace->requestConfirmLoad(Action::OpenScene);
        f.frame();
        require(f.press("Unsaved changes", "Cancel").loadDecision == Decision::Cancel, "Cancel decision was lost");
        f.frame();
        f.workspace->requestConfirmLoad(Action::NewScene);
        f.frame();
        require(f.press("Unsaved changes", "Discard").loadDecision == Decision::DiscardChanges, "Discard decision was lost");
        f.frame();
        f.workspace->requestOpenSaveAs();
        f.frame();
        f.workspace->setSaveAsError("Write failed");
        f.frame();
        require(f.topIs("Save scene as"), "Save error closed the dialog");
        require(f.press("Save scene as", "Cancel").saveAsCancelled, "Cancelling Save As was not reported");
        f.frame();
        require(!f.workspace->hasOpenPopup(), "Cancelled Save As left a blocking dialog");
    }
}

int main() {
    int failures = 0;
    failures += run("ImGui Ctrl+Shift+Z and Ctrl+Y redo shortcuts", redoKeyboardShortcuts);
    failures += run("ImGui physics fields, complete snapshots and single-edit undo", physicsInspectorHistory);
    failures += run("ImGui physics save, selection, hidden panel, Escape and Play", physicsInspectorLifecycle);
    failures += run("ImGui Physics checkbox, incomplete components, Play and undo", physicsInspectorAddRemove);
    failures += run("ImGui shape, Box dimensions, Static controls and mouse drag", physicsInspectorShapesAndDrag);
    failures += run("ImGui save and nested overwrite confirmation during Play", saveAndOverwriteDialogs);
    failures += run("ImGui Cancel, Discard and failed-save cancellation", cancelAndDiscardDialogs);
    return failures == 0 ? 0 : 1;
}
