#include "simulation/SimulationController.hpp"
#include "SceneWriteTestSupport.hpp"

#include <algorithm>
#include <nlohmann/json.hpp>

#include "commands/CommandHistory.hpp"
#include "commands/EntityCommands.hpp"
#include "commands/EntitySnapshot.hpp"
#include "core/EngineContext.hpp"
#include "documents/DocumentController.hpp"
#include "systems/PhysicsSystem.hpp"
#include "systems/RenderSystem.hpp"

namespace {
    namespace components = AetherEngine::components;
    namespace assets = AetherEngine::assets;
    namespace systems = AetherEngine::systems;
    using AetherEngine::core::EngineContext;
    using AetherEngine::core::EngineError;
    using AetherEngine::scene::EntityId;
    using AetherEngine::scene::Scene;
    using AetherEditor::simulation::SimulationController;
    using scene_write_tests::require;
    using Result = std::expected<void, EngineError>;
    constexpr EntityId Rich{100, 101};
    constexpr EntityId Bare{102, 103};
    constexpr EntityId RuntimeOnly{104, 105};

    static_assert(noexcept(std::declval<systems::PhysicsSystem&>().clearSceneBodies()));

    struct Fixture {
        scene_write_tests::Fixture files;
        AetherEngine::core::EnginePaths paths{.assetsRoot = files.root / "assets", .scenesRoot = files.scenes};
        AetherEngine::logging::Logger logger;
        AetherEngine::resources::ResourceManager::ResourceManagerPtr resources =
            AetherEngine::resources::ResourceManager::createResourceManager(paths.assetsRoot);
        std::unique_ptr<assets::AssetManager> assets;
        systems::SceneAssetBindingSystem::SceneAssetBindingSystemPtr binding;
        systems::InputSystem::InputSystemPtr input = systems::InputSystem::createInputSystem();
        systems::RenderSystem::RenderSystemPtr renderer = systems::RenderSystem::createRenderSystem();
        systems::SceneSerializer::SceneSerializerPtr serializer = systems::SceneSerializer::createSceneSerializer(files.scenes);
        AetherEngine::Window::Ptr window = AetherEngine::Window::createWindow();
        Scene::ScenePtr active = Scene::createScene("Unsaved edit scene");
        SimulationController::SimulationControllerPtr controller = SimulationController::createSimulationController();
        int reads = 0;
        int exchanges = 0;
        bool bindScenes = false;
        std::function<Result(Scene::ScenePtr&)> beforeExchange;

        Fixture() {
            std::filesystem::create_directories(paths.assetsRoot / "registry");
            scene_write_tests::put(paths.assetsRoot / "registry/runtime-assets.json", R"({"version":1,"assets":[]})");
            auto created = assets::AssetManager::create(paths.assetsRoot, *resources,
                {assets::OperatingSystem::Windows, assets::Architecture::X64, assets::GraphicsBackend::Direct3D11});
            require(created.has_value(), "Could not create headless asset manager");
            assets = std::move(*created);
            binding = systems::SceneAssetBindingSystem::create(*assets);
        }

        auto context() -> EngineContext {
            return {
                .logger = logger, .assets = *assets, .input = *input, .renderer = *renderer,
                .sceneSerializer = *serializer, .sceneAssetBinding = *binding,
                .window = *window, .paths = paths,
                .requestQuit = [] {},
                .getScene = [this]() -> Scene& { ++reads; return *active; },
                .replaceScene = [](Scene::ScenePtr) -> Result {
                    throw std::runtime_error("Simulation must exchange, not replace, the edit scene");
                },
                // Exercise the EngineContext contract: prepare first, swap only on success.
                .exchangeScene = [this](Scene::ScenePtr& other) -> Result {
                    ++exchanges;
                    require(other != nullptr, "Exchange received a null scene");
                    if (beforeExchange) {
                        auto result = beforeExchange(other);
                        if (!result) return result;
                    }
                    if (bindScenes) {
                        auto bound = binding->bindScene(*other);
                        if (!bound) return std::unexpected(EngineError{5, bound.error().message});
                    }
                    active.swap(other);
                    return {};
                }
            };
        }

        auto contents(Scene& scene) -> nlohmann::json {
            require(serializer->serializeScene(scene, "Snapshot").has_value(), "Could not serialize scene");
            auto result = nlohmann::json::parse(scene_write_tests::get(files.scenes / "Snapshot.scene.json"));
            auto& entities = result.at("entities");
            std::sort(entities.begin(), entities.end(), [](const auto& a, const auto& b) {
                return a.at("id").template get<std::string>() < b.at("id").template get<std::string>();
            });
            return result;
        }
    };

    auto populate(Scene& scene) -> void {
        const auto entity = scene.createEntityWithId(Rich, "Camera and body");
        auto& transform = scene.addComponent<components::TransformComponent>(entity).transform;
        transform.position = {1.25f, -2.0f, 3.5f};
        transform.rotation = AetherEngine::math::Quat::fromEulerXYZDegrees({15, 25, 35}).normalized();
        transform.scale = {1.5f, 2.0f, 2.5f};
        scene.addComponent<components::MeshComponent>(entity).asset.id = {1, 2};
        scene.addComponent<components::MaterialComponent>(entity) = {
            .asset = {{3, 4}}, .baseColor = {0.2f, 0.4f, 0.6f, 0.8f}
        };
        scene.addComponent<components::CameraComponent>(entity) = {
            .enabled = true, .projection = components::ProjectionType::Orthographic,
            .fovYDegrees = 75, .nearPlane = 0.125f, .farPlane = 450, .orthographicHeight = 12.5f,
            .clearFlags = components::ClearDepth, .clearColor = 0x112233ff, .priority = 9, .layerMask = 0x55
        };
        scene.addComponent<components::ColliderComponent>(entity) = {
            .type = components::ColliderType::Capsule, .size = {2, 3, 4},
            .radius = 0.35f, .height = 2.25f, .trigger = true
        };
        scene.addComponent<components::RigidbodyComponent>(entity) = {.dynamic = true, .mass = 3.5f, .useGravity = false};
        scene.setActiveCamera(entity);
        scene.createEntityWithId(Bare, "Identity only");
        const auto inactive = scene.createEntity("Inactive camera");
        scene.addComponent<components::CameraComponent>(inactive).priority = 4;
    }

    auto roundTrip() -> void {
        Fixture f;
        auto ctx = f.context();
        populate(*f.active);
        auto* edit = f.active.get();
        const auto original = *edit->findEntityById(Rich);
        const auto before = f.contents(*edit);
        for (int cycle = 0; cycle < 3; ++cycle) {
            require(f.controller->start(ctx).has_value() && f.controller->isRunning(), "Start failed");
            require(f.active.get() != edit && f.active->getName() == edit->getName(), "Start did not create a named runtime clone");
            require(f.contents(*f.active) == before, "Clone lost component fields, entity IDs or active camera");
            require(f.active->getActiveCamera().scene == f.active.get(), "Runtime camera points at the edit scene");
            require(!f.active->hasComponent<components::TransformComponent>(*f.active->findEntityById(Bare)),
                "Clone invented an optional component");

            auto lifetime = std::make_shared<int>(1);
            std::weak_ptr<int> runtimeLifetime = lifetime;
            const auto runtimeEntity = *f.active->findEntityById(Rich);
            f.active->addComponent<std::shared_ptr<int>>(runtimeEntity, std::move(lifetime));
            f.active->getComponent<components::TransformComponent>(runtimeEntity).transform.position = {80, 90, 100};
            f.active->destroyEntity(*f.active->findEntityById(Bare));
            f.active->createEntityWithId(RuntimeOnly, "Runtime only");
            require(f.contents(*edit) == before && edit->isAlive(original), "Runtime mutations leaked into the edit scene");
            require(f.controller->stop(ctx).has_value() && !f.controller->isRunning(), "Stop failed");
            require(f.active.get() == edit && edit->isAlive(original) && f.contents(*edit) == before,
                "Stop failed to restore the exact original scene and handles");
            require(runtimeLifetime.expired(), "Stop retained the runtime clone");
        }
    }

    auto guardsAndEmptyScene() -> void {
        Fixture f;
        auto ctx = f.context();
        auto* edit = f.active.get();
        require(!f.controller->isRunning() && !f.controller->stop(ctx), "Initial Stop was accepted");
        require(f.exchanges == 0 && f.reads == 0, "Stopped guard called engine callbacks");
        require(f.controller->start(ctx).has_value() && f.active->isEmpty(), "Could not start an empty scene");
        auto* runtime = f.active.get();
        require(!f.controller->start(ctx) && f.controller->isRunning() && f.active.get() == runtime,
            "Repeated Start replaced the saved edit scene");
        require(f.exchanges == 1 && f.reads == 1, "Running guard called engine callbacks");
        require(f.controller->stop(ctx).has_value() && f.active.get() == edit, "Empty scene Stop failed");
        require(!f.controller->stop(ctx) && f.exchanges == 2, "Repeated Stop exchanged scenes");
    }

    auto callbackFailures() -> void {
        Fixture f;
        auto ctx = f.context();
        populate(*f.active);
        auto* edit = f.active.get();
        const auto before = f.contents(*edit);
        const auto getScene = ctx.getScene;
        const auto exchange = ctx.exchangeScene;
        ctx.getScene = {};
        require(!f.controller->start(ctx), "Missing scene callback was accepted");
        ctx.getScene = []() -> Scene& { throw std::runtime_error("scene access failed"); };
        auto result = f.controller->start(ctx);
        require(!result && result.error().message.find("scene access failed") != std::string::npos,
            "Scene access exception escaped or lost its diagnostic");
        ctx.getScene = []() -> Scene& { throw 42; };
        require(!f.controller->start(ctx), "Unknown scene access exception escaped");
        ctx.getScene = getScene;
        ctx.exchangeScene = {};
        require(!f.controller->start(ctx) && f.exchanges == 0, "Missing exchange callback was accepted");
        ctx.exchangeScene = exchange;

        for (int failure = 0; failure < 3; ++failure) {
            f.beforeExchange = [failure](Scene::ScenePtr&) -> Result {
                if (failure == 1) throw std::runtime_error("exchange failed");
                if (failure == 2) throw 42;
                return std::unexpected(EngineError{73, "exchange rejected"});
            };
            result = f.controller->start(ctx);
            require(!result && !f.controller->isRunning() && f.active.get() == edit && f.contents(*edit) == before,
                "Failed Start lost or changed the edit scene/state");
            if (failure == 0) require(result.error().code == 73 && result.error().message == "exchange rejected",
                "Start did not propagate the engine error");
            if (failure == 1) require(result.error().message.find("exchange failed") != std::string::npos,
                "Start lost the exchange exception diagnostic");
        }
        f.beforeExchange = {};
        require(f.controller->start(ctx).has_value(), "Start could not recover after callback failures");
        auto* runtime = f.active.get();
        ctx.exchangeScene = {};
        require(!f.controller->stop(ctx) && f.controller->isRunning() && f.active.get() == runtime,
            "Missing Stop callback lost the running scene");
        ctx.exchangeScene = exchange;
        for (int failure = 0; failure < 3; ++failure) {
            f.beforeExchange = [failure, edit](Scene::ScenePtr& other) -> Result {
                require(other.get() == edit, "Stop no longer owns the original edit scene");
                if (failure == 1) throw std::runtime_error("exchange failed");
                if (failure == 2) throw 42;
                return std::unexpected(EngineError{73, "exchange rejected"});
            };
            result = f.controller->stop(ctx);
            require(!result && f.controller->isRunning() && f.active.get() == runtime && f.contents(*edit) == before,
                "Failed Stop lost the edit scene or changed the running state");
            if (failure == 0) require(result.error().code == 73 && result.error().message == "exchange rejected",
                "Stop did not propagate the engine error");
            if (failure == 1) require(result.error().message.find("exchange failed") != std::string::npos,
                "Stop lost the exchange exception diagnostic");
        }
        f.beforeExchange = {};
        require(f.controller->stop(ctx).has_value() && f.active.get() == edit, "Stop could not recover after callback failures");
    }

    auto cloneFailure() -> void {
        Fixture f;
        auto ctx = f.context();
        const auto entity = f.active->createEntityWithId(Rich, "Broken identity");
        auto& identity = f.active->getComponent<components::IdentityComponent>(entity);
        identity.id = {}; // Simulate an invalid edit scene without allocation-failure hooks.
        auto* edit = f.active.get();
        require(!f.controller->start(ctx) && !f.controller->isRunning() && f.exchanges == 0 && f.active.get() == edit,
            "Clone failure exchanged or destroyed the edit scene");
        identity.id = Rich;
        require(f.controller->start(ctx).has_value() && f.controller->stop(ctx).has_value(),
            "Controller could not recover after repairing the edit scene");
    }

    auto bindingFailures() -> void {
        Fixture f;
        auto ctx = f.context();
        f.bindScenes = true;
        auto* edit = f.active.get();
        const auto entity = edit->createEntityWithId(Rich, "Missing asset");
        require(f.binding->bindScene(*edit).has_value(), "Initial binding failed");
        edit->addComponent<components::MeshComponent>(entity).asset.id = {11, 12};
        require(!f.controller->start(ctx) && !f.controller->isRunning() && f.active.get() == edit,
            "Asset-binding failure changed Start state/ownership");
        edit->removeComponent<components::MeshComponent>(entity);
        require(f.binding->synchronize(*edit).has_value(), "Failed Start changed the active asset binding");
        require(f.controller->start(ctx).has_value(), "Start failed after correcting the missing asset");
        auto* runtime = f.active.get();
        // Make the saved scene unbindable to exercise the real binder's Stop failure.
        edit->addComponent<components::MeshComponent>(entity).asset.id = {11, 12};
        require(!f.controller->stop(ctx) && f.controller->isRunning() && f.active.get() == runtime,
            "Asset-binding failure lost the saved scene or stopped simulation");
        require(f.binding->synchronize(*runtime).has_value(), "Failed Stop changed the active asset binding");
        edit->removeComponent<components::MeshComponent>(entity);
        require(f.controller->stop(ctx).has_value() && f.active.get() == edit && f.binding->synchronize(*edit).has_value(),
            "Stop retry did not restore the original scene and binding");
    }

    auto documentAndHistory() -> void {
        Fixture f;
        auto ctx = f.context();
        auto* edit = f.active.get();
        edit->createEntityWithId(Rich, "Before");
        AetherEditor::commands::CommandHistory history;
        history.markSaved();
        require(history.execute(std::make_unique<AetherEditor::commands::RenameEntityCommand>(Rich, "Before", "Edited"), *edit),
            "Could not establish an unsaved editor command");
        auto document = AetherEditor::docs::DocumentController::createDocumentController();
        document->setScene(*edit);
        document->setFileStem("SavedScene");
        document->dirty(history.isDirty());
        require(f.controller->start(ctx).has_value(), "Start failed with unsaved edits");
        f.active->destroyEntity(*f.active->findEntityById(Rich));
        f.beforeExchange = [](Scene::ScenePtr&) -> Result { return std::unexpected(EngineError{5, "temporary failure"}); };
        require(!f.controller->stop(ctx), "Expected Stop failure");
        require(document->getDocument().scene == edit && document->getDocument().fileStem == "SavedScene" &&
                document->getDocument().dirty && history.isDirty() && history.canUndo(),
            "Simulation changed the edit document or unsaved history");
        f.beforeExchange = {};
        require(f.controller->stop(ctx).has_value() && document->getDocument().scene == f.active.get(),
            "Stop did not restore the editor document's scene");
        require(history.undo(*f.active) && !history.isDirty() &&
                f.active->getComponent<components::IdentityComponent>(*f.active->findEntityById(Rich)).name == "Before",
            "Original editor undo history no longer works after Stop");
        require(history.redo(*f.active) && history.isDirty(), "Original editor redo history no longer works after Stop");
    }
}

int main() {
    int failures = 0;
    failures += scene_write_tests::run("clone all scene data, isolate runtime changes and restore original ownership", roundTrip);
    failures += scene_write_tests::run("empty scenes and repeated Start/Stop guards", guardsAndEmptyScene);
    failures += scene_write_tests::run("missing callbacks, returned errors, exceptions and retries", callbackFailures);
    failures += scene_write_tests::run("clone failure preserves the edit scene and can be retried", cloneFailure);
    failures += scene_write_tests::run("real asset binding failures preserve active scene and binding", bindingFailures);
    failures += scene_write_tests::run("document pointer, unsaved edits and undo/redo survive simulation", documentAndHistory);
    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
