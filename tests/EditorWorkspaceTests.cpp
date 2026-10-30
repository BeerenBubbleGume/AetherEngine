#include "SceneWriteTestSupport.hpp"
#include "ui/EditorWorkspace.hpp"
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
    };

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
    failures += run("ImGui save and nested overwrite confirmation during Play", saveAndOverwriteDialogs);
    failures += run("ImGui Cancel, Discard and failed-save cancellation", cancelAndDiscardDialogs);
    return failures == 0 ? 0 : 1;
}
