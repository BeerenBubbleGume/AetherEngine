#include "SceneWriteTestSupport.hpp"
#include "documents/DocumentController.hpp"
#include "simulation/SimulationController.hpp"
#include "components/TransformComponent.hpp"
#include "systems/RenderSystem.hpp"

namespace {
    using namespace scene_write_tests;
    namespace ae = AetherEngine;
    using Scene = ae::scene::Scene;
    using Action = AetherEditor::core::DocumentActionKind;
    using Decision = AetherEditor::core::SceneLoadDecision;
    using Result = std::expected<void, ae::core::EngineError>;

    struct DocumentFixture {
        Fixture files;
        ae::core::EnginePaths paths{.assetsRoot = files.root / "assets", .scenesRoot = files.scenes};
        ae::logging::Logger logger;
        ae::resources::ResourceManager::ResourceManagerPtr resources = ae::resources::ResourceManager::createResourceManager(paths.assetsRoot);
        std::unique_ptr<ae::assets::AssetManager> assets;
        ae::systems::SceneAssetBindingSystem::SceneAssetBindingSystemPtr binding;
        ae::systems::InputSystem::InputSystemPtr input = ae::systems::InputSystem::createInputSystem();
        ae::systems::RenderSystem::RenderSystemPtr renderer = ae::systems::RenderSystem::createRenderSystem();
        ae::systems::SceneSerializer::SceneSerializerPtr serializer = ae::systems::SceneSerializer::createSceneSerializer(files.scenes);
        ae::Window::Ptr window = ae::Window::createWindow();
        Scene::ScenePtr scene = Scene::createScene("Untitled");
        AetherEditor::docs::DocumentController::DocumentControllerPtr document = AetherEditor::docs::DocumentController::createDocumentController();

        DocumentFixture() {
            fs::create_directories(paths.assetsRoot / "registry");
            put(paths.assetsRoot / "registry/runtime-assets.json", R"({"version":1,"assets":[]})");
            auto created = ae::assets::AssetManager::create(paths.assetsRoot, *resources,
                {ae::assets::OperatingSystem::Windows, ae::assets::Architecture::X64, ae::assets::GraphicsBackend::Direct3D11});
            require(created.has_value(), "Asset manager creation failed");
            assets = std::move(*created);
            binding = ae::systems::SceneAssetBindingSystem::create(*assets);
            binding->setAllowMissingAssets(true);
            document->setScene(*scene);
            document->dirty(true);
        }

        auto context() -> ae::core::EngineContext {
            return {
                .logger = logger, .assets = *assets, .input = *input, .renderer = *renderer,
                .sceneSerializer = *serializer, .sceneAssetBinding = *binding, .window = *window, .paths = paths,
                .requestQuit = [] {}, .getScene = [this]() -> Scene& { return *scene; },
                .replaceScene = [this](Scene::ScenePtr next) -> Result {
                    auto bound = binding->bindScene(*next);
                    if (!bound) return std::unexpected(ae::core::EngineError{5, bound.error().message});
                    scene = std::move(next);
                    return {};
                },
                .exchangeScene = [this](Scene::ScenePtr& other) -> Result {
                    auto bound = binding->bindScene(*other);
                    if (!bound) return std::unexpected(ae::core::EngineError{5, bound.error().message});
                    scene.swap(other);
                    return {};
                }
            };
        }
    };

    void decisions() {
        for (const auto kind : {Action::Quit, Action::NewScene, Action::OpenScene}) {
            DocumentFixture f;
            require(f.document->requestAction({kind, "Next"}), "Action was not accepted");
            require(f.document->needsConfirmation() && !f.document->takeReadyAction(), "Dirty document escaped confirmation");
            require(!f.document->requestAction({Action::Quit, {}}), "Repeated close replaced a pending action");
            f.document->decideAction(Decision::Cancel);
            require(!f.document->hasPendingAction() && !f.document->takeReadyAction(), "Cancel left an executable action");
            require(f.document->getDocument().dirty, "Cancel cleared dirty state");
            f.document->requestAction({kind, "Next"});
            f.document->decideAction(Decision::DiscardChanges);
            const auto action = f.document->takeReadyAction();
            require(action && action->kind == kind && action->sceneName == "Next", "Discard changed the requested destination");
            require(!f.document->takeReadyAction(), "Action could execute twice");
            f.document->dirty(false);
            f.document->requestAction({kind, "Clean"});
            require(!f.document->needsConfirmation() && f.document->takeReadyAction().has_value(), "Clean document was blocked");
        }
    }

    void saveAsAndConflicts() {
        DocumentFixture f;
        auto ctx = f.context();
        auto entity = f.scene->createEntity("Original");
        const auto id = f.scene->getComponent<ae::components::IdentityComponent>(entity).id;
        require(f.document->saveAs(ctx, "First").has_value(), "Save As did not create a new file");
        const auto first = get(f.files.scenes / "First.scene.json");
        f.scene->getComponent<ae::components::IdentityComponent>(entity).name = "Edited";
        f.document->dirty(true);
        f.document->requestAction({Action::Quit, {}});
        f.document->decideAction(Decision::SaveChanges);
        put(f.files.scenes / "Existing.scene.json", "preserve this file");
        const auto conflict = f.document->saveAs(ctx, "Existing");
        require(!conflict && conflict.error().code == ae::systems::SceneFileAlreadyExistsError, "Save As conflict was not typed");
        require(get(f.files.scenes / "Existing.scene.json") == "preserve this file", "Unconfirmed overwrite changed bytes");
        require(f.document->getDocument().fileStem == "First" && f.document->getDocument().dirty,
            "Conflict changed document filename or dirty state");
        require(f.document->awaitingSave() && !f.document->takeReadyAction(), "Conflict allowed exit");
        f.document->cancelAction();
        require(!f.document->takeReadyAction(), "Cancelling Save As allowed exit");
        require(f.document->saveAs(ctx, "Existing", true).has_value(), "Confirmed overwrite failed");
        auto loaded = f.serializer->deserializeScene("Existing");
        require(loaded && (*loaded)->findEntityById(id).has_value(), "Replacement is not a readable scene");
        require((*loaded)->getComponent<ae::components::IdentityComponent>(*(*loaded)->findEntityById(id)).name == "Edited",
            "Replacement saved old data");
        require(get(f.files.scenes / "First.scene.json") == first, "Save As modified the previous file");
        require(f.document->getDocument().fileStem == "Existing" && !f.document->getDocument().dirty, "Save As did not adopt its path");
        f.document->dirty(true);
        require(f.document->save(ctx).has_value(), "Normal Save incorrectly required overwrite approval");
    }

    void saveFailureAndRetry() {
        DocumentFixture f;
        auto ctx = f.context();
        f.document->requestAction({Action::OpenScene, "Next"});
        f.document->decideAction(Decision::SaveChanges);
        require(!f.document->save(ctx), "Untitled Save unexpectedly succeeded");
        require(f.document->awaitingSave() && !f.document->takeReadyAction(), "Missing filename allowed transition");
        require(!f.document->saveAs(ctx, "../escape"), "Unsafe filename accepted");
        fs::create_directory(f.files.scenes / "Blocked.scene.json");
        require(!f.document->saveAs(ctx, "Blocked", true), "Directory replaced by a file");
        require(f.document->getDocument().fileStem.empty() && f.document->getDocument().dirty && !f.document->takeReadyAction(),
            "Failed save changed document or released transition");
        require(f.document->saveAs(ctx, "Recovered").has_value(), "Retry failed");
        const auto action = f.document->takeReadyAction();
        require(action && action->kind == Action::OpenScene && action->sceneName == "Next", "Successful retry lost pending destination");
        require(!f.document->getDocument().dirty, "Successful save retained dirty state");
    }

    void failedOpenPreservesDocument() {
        DocumentFixture f;
        auto ctx = f.context();
        require(f.document->saveAs(ctx, "Original").has_value(), "Setup save failed");
        f.document->dirty(true);
        auto* original = f.scene.get();
        put(f.files.scenes / "Broken.scene.json", "{invalid");
        require(!f.document->load(ctx, "Broken"), "Invalid scene opened");
        require(f.scene.get() == original && f.document->getDocument().scene == original &&
            f.document->getDocument().fileStem == "Original" && f.document->getDocument().dirty,
            "Failed Open changed the current document");
    }

    void closeDuringPlaySavesAuthoringScene() {
        DocumentFixture f;
        auto ctx = f.context();
        const auto entity = f.scene->createEntity("Body");
        const auto id = f.scene->getComponent<ae::components::IdentityComponent>(entity).id;
        f.scene->addComponent<ae::components::TransformComponent>(entity).transform.position.y = 12;
        auto simulation = AetherEditor::simulation::SimulationController::createSimulationController();
        auto* edit = f.scene.get();
        require(simulation->start(ctx).has_value(), "Play failed");
        f.scene->getComponent<ae::components::TransformComponent>(*f.scene->findEntityById(id)).transform.position.y = -50;
        f.document->requestAction({Action::Quit, {}});
        f.document->decideAction(Decision::Cancel);
        require(simulation->isRunning() && f.document->getDocument().scene == edit, "Cancel during Play lost authoring scene");
        f.document->requestAction({Action::Quit, {}});
        f.document->decideAction(Decision::SaveChanges);
        require(f.document->saveAs(ctx, "SavedDuringPlay").has_value(), "Save during close failed");
        const auto action = f.document->takeReadyAction();
        require(action && action->kind == Action::Quit, "Successful save did not release exit");
        auto loaded = f.serializer->deserializeScene("SavedDuringPlay");
        require(loaded && (*loaded)->getComponent<ae::components::TransformComponent>(*(*loaded)->findEntityById(id)).transform.position.y == 12,
            "Close saved the runtime position instead of authoring data");
        require(simulation->stop(ctx).has_value() && f.scene.get() == edit, "Stop failed after close/save");
    }
}

int main() {
    int failures = 0;
    failures += run("Save/Discard/Cancel and repeated transition requests", decisions);
    failures += run("Save As creates, requires confirmation, preserves old file and adopts new path", saveAsAndConflicts);
    failures += run("failed Save As blocks transition and supports retry", saveFailureAndRetry);
    failures += run("failed Open preserves document", failedOpenPreservesDocument);
    failures += run("close during Play saves retained authoring data", closeDuringPlaySavesAuthoringScene);
    return failures == 0 ? 0 : 1;
}
