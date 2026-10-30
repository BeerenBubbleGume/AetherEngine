#include "SceneWriteTestSupport.hpp"

#include <algorithm>
#include <nlohmann/json.hpp>

#include "commands/CommandHistory.hpp"
#include "commands/ComponentCommands.hpp"
#include "commands/EntityCommands.hpp"
#include "core/EntityName.hpp"
#include "systems/SceneSerializer.hpp"

namespace {
    namespace commands = AetherEditor::commands;
    namespace components = AetherEngine::components;
    namespace assets = AetherEngine::assets;
    using AetherEngine::scene::Entity;
    using AetherEngine::scene::EntityId;
    using AetherEngine::scene::Scene;
    using AetherEngine::systems::SceneSerializer;
    using Json = nlohmann::json;
    using scene_write_tests::require;
    constexpr EntityId Source{100, 101};
    constexpr EntityId Copy{102, 103};
    constexpr assets::AssetId MeshId{1, 2};
    constexpr assets::AssetId MaterialId{3, 4};

    auto sceneJson(Scene& scene, SceneSerializer& serializer, const scene_write_tests::Fixture& files,
                   const std::string& stem) -> Json {
        require(serializer.serializeScene(scene, stem).has_value(), "Could not serialize test scene");
        auto result = Json::parse(scene_write_tests::get(files.scenes / (stem + ".scene.json")));
        auto& entities = result.at("entities");
        std::sort(entities.begin(), entities.end(), [](const Json& left, const Json& right) {
            return left.at("id").get<std::string>() < right.at("id").get<std::string>();
        });
        return result;
    }

    auto richEntity(Scene& scene) -> Entity {
        const auto entity = scene.createEntityWithId(Source, "Rich entity");
        auto& transform = scene.addComponent<components::TransformComponent>(entity).transform;
        transform.position = {1.25f, -2.0f, 3.5f};
        transform.rotation = AetherEngine::math::Quat::fromEulerXYZDegrees({15.0f, 25.0f, 35.0f}).normalized();
        transform.scale = {1.5f, 2.0f, 2.5f};
        scene.addComponent<components::MeshComponent>(entity).asset.id = MeshId;
        scene.addComponent<components::MaterialComponent>(entity) = {.asset = {MaterialId}, .baseColor = {0.2f, 0.4f, 0.6f, 0.8f}};
        scene.addComponent<components::CameraComponent>(entity) = {
            .enabled = true, .projection = components::ProjectionType::Orthographic,
            .fovYDegrees = 75.0f, .nearPlane = 0.125f, .farPlane = 450.0f, .orthographicHeight = 12.5f,
            .clearFlags = components::ClearDepth, .clearColor = 0x112233ff, .priority = 9, .layerMask = 0x55
        };
        scene.addComponent<components::ColliderComponent>(entity) = {
            .type = components::ColliderType::Capsule, .size = {2.0f, 3.0f, 4.0f},
            .radius = 0.35f, .height = 2.25f, .trigger = true
        };
        scene.addComponent<components::RigidbodyComponent>(entity) = {.dynamic = true, .mass = 3.5f, .useGravity = false};
        return entity;
    }

    auto deletionRoundTrip() -> void {
        scene_write_tests::Fixture files;
        auto serializer = SceneSerializer::createSceneSerializer(files.scenes);
        auto scene = Scene::createScene("Deletion");
        const auto source = richEntity(*scene);
        const auto sentinel = scene->createEntity("Sentinel");
        scene->setActiveCamera(source);
        const auto before = sceneJson(*scene, *serializer, files, "Before");
        commands::CommandHistory history;
        require(history.execute(std::make_unique<commands::DeleteEntityCommand>(Source), *scene) && history.isDirty(), "Delete did not enter history");
        require(!scene->findEntityById(Source) && !scene->isAlive(source) && scene->isAlive(sentinel) &&
                !scene->getActiveCamera().isValid(), "Delete left a stale entity/camera or removed another object");
        sceneJson(*scene, *serializer, files, "Deleted");
        auto loaded = serializer->deserializeScene("Deleted");
        require(loaded && !(*loaded)->findEntityById(Source), "Deleted entity survived scene reload");
        for (int repeat = 0; repeat < 3; ++repeat) {
            require(history.undo(*scene) && !history.isDirty(), "Undo deletion failed");
            require(sceneJson(*scene, *serializer, files, "Restored") == before,
                    "Undo lost a component, field, stable ID or active camera");
            require(!scene->isAlive(source), "Undo resurrected the destroyed EnTT handle");
            if (repeat < 2) require(history.redo(*scene) && !scene->findEntityById(Source), "Redo deletion failed");
        }
        loaded = serializer->deserializeScene("Restored");
        require(loaded && sceneJson(**loaded, *serializer, files, "Reloaded") == before, "Restored scene failed a save/load round trip");
    }

    auto optionalComponents() -> void {
        for (int kind = 0; kind < 3; ++kind) {
            scene_write_tests::Fixture files;
            auto serializer = SceneSerializer::createSceneSerializer(files.scenes);
            auto scene = Scene::createScene("Optional components");
            const auto source = scene->createEntityWithId(Source, "Ground");
            if (kind == 1) {
                scene->addComponent<components::TransformComponent>(source).transform.position = {0, -2, 0};
                scene->addComponent<components::ColliderComponent>(source).size = {12, 0.5f, 6};
                scene->addComponent<components::RigidbodyComponent>(source) = {.dynamic = false, .mass = 1, .useGravity = false};
            } else if (kind == 2) {
                scene->addComponent<components::CameraComponent>(source).priority = 7;
                const auto other = scene->createEntity("Active camera");
                scene->addComponent<components::CameraComponent>(other);
                scene->setActiveCamera(other);
            }
            const auto before = sceneJson(*scene, *serializer, files, "Before");
            commands::CommandHistory history;
            require(history.execute(std::make_unique<commands::DeleteEntityCommand>(Source), *scene) && history.undo(*scene), "Delete/undo optional components failed");
            require(sceneJson(*scene, *serializer, files, "After") == before, "Undo invented a missing component or changed another active camera");
            require(history.execute(std::make_unique<commands::DuplicateEntityCommand>(Source, Copy), *scene), "Duplicate optional components failed");
            const auto clone = *scene->findEntityById(Copy);
            const auto original = *scene->findEntityById(Source);
            require(scene->hasComponent<components::TransformComponent>(clone) == scene->hasComponent<components::TransformComponent>(original) &&
                    scene->hasComponent<components::CameraComponent>(clone) == scene->hasComponent<components::CameraComponent>(original) &&
                    !scene->hasComponent<components::MeshComponent>(clone) && !scene->hasComponent<components::MaterialComponent>(clone),
                    "Duplicate invented missing components");
        }
    }

    auto duplicateRoundTrip() -> void {
        scene_write_tests::Fixture files;
        auto serializer = SceneSerializer::createSceneSerializer(files.scenes);
        auto scene = Scene::createScene("Duplication");
        const auto source = richEntity(*scene);
        scene->setActiveCamera(source);
        const auto before = sceneJson(*scene, *serializer, files, "Before");
        commands::CommandHistory history;
        require(history.execute(std::make_unique<commands::DuplicateEntityCommand>(Source, Copy), *scene) && history.isDirty(), "Duplicate did not enter history");
        const auto clone = *scene->findEntityById(Copy);
        auto expected = before;
        auto copiedBody = before.at("entities").at(0);
        copiedBody["id"] = AetherEngine::scene::toString(Copy);
        copiedBody["name"] = "Rich entity Copy";
        expected["entities"].push_back(copiedBody);
        require(sceneJson(*scene, *serializer, files, "Duplicated") == expected,
                "Duplicate changed source/active camera, lost component data or changed asset references");
        auto loaded = serializer->deserializeScene("Duplicated");
        require(loaded && sceneJson(**loaded, *serializer, files, "Reloaded") == expected, "Duplicate failed a save/load round trip");
        require(history.undo(*scene) && !history.isDirty() && !scene->isAlive(clone) &&
                sceneJson(*scene, *serializer, files, "Undone") == before, "Undo duplicate changed the source or saved state");
        // Redo must replay the captured copy, independently of the source's lifetime.
        scene->destroyEntity(source);
        require(history.redo(*scene) && scene->findEntityById(Copy) && !scene->findEntityById(Source) && !scene->getActiveCamera().isValid(),
                "Redo duplicate depended on source existence or activated the copied camera");
        require(!scene->isAlive(clone), "Redo duplicate resurrected a stale handle");
        const auto redone = sceneJson(*scene, *serializer, files, "Redone");
        require(redone.at("entities").size() == 1 && redone.at("entities").at(0) == copiedBody,
                "Redo duplicate changed the captured ID/name/components");
    }

    auto duplicateNames() -> void {
        scene_write_tests::Fixture files;
        auto serializer = SceneSerializer::createSceneSerializer(files.scenes);
        auto scene = Scene::createScene("Copy names");
        std::string longName;
        for (int index = 0; index < 512; ++index) longName += "\xd1\x8f";
        scene->createEntityWithId(Source, longName);
        commands::CommandHistory history;
        require(history.execute(std::make_unique<commands::DuplicateEntityCommand>(Source, Copy), *scene), "Duplicate long UTF-8 name failed");
        const auto& firstName = scene->getComponent<components::IdentityComponent>(*scene->findEntityById(Copy)).name;
        require(AetherEditor::core::isValidEntityName(firstName) && firstName.ends_with(" Copy"), "Copy name exceeded serializer limits");
        const EntityId second{104, 105};
        require(history.execute(std::make_unique<commands::DuplicateEntityCommand>(Source, second), *scene), "Repeated duplication failed");
        const auto& secondName = scene->getComponent<components::IdentityComponent>(*scene->findEntityById(second)).name;
        require(secondName != firstName && secondName.ends_with(" Copy (1)"), "Copy names collided");
        sceneJson(*scene, *serializer, files, "Unicode");
        require(serializer->deserializeScene("Unicode").has_value(), "Copy-name truncation split a UTF-8 character");
        const auto unnamed = scene->createEntity("");
        const auto unnamedId = scene->getComponent<components::IdentityComponent>(unnamed).id;
        const EntityId third{106, 107};
        require(history.execute(std::make_unique<commands::DuplicateEntityCommand>(unnamedId, third), *scene) &&
                scene->getComponent<components::IdentityComponent>(*scene->findEntityById(third)).name == "Entity Copy", "Unnamed entity copy has no useful name");
    }

    auto guardFailures() -> void {
        auto scene = Scene::createScene("Failure guards");
        const auto source = richEntity(*scene);
        commands::CommandHistory history;
        require(!commands::captureEntity(*scene, Copy), "Captured a missing entity");
        require(!history.execute(std::make_unique<commands::DeleteEntityCommand>(Copy), *scene) && !history.canUndo(), "Missing deletion entered history");
        for (const auto target : {EntityId{}, Source}) {
            require(!history.execute(std::make_unique<commands::DuplicateEntityCommand>(Source, target), *scene), "Invalid duplicate ID was accepted");
        }
        require(!history.execute(std::make_unique<commands::DuplicateEntityCommand>(Copy, EntityId{108, 109}), *scene) && !history.isDirty(), "Missing source entered history");
        const auto snapshot = *commands::captureEntity(*scene, Source);
        require(!commands::restoreEntity(*scene, snapshot), "Restore overwrote an existing entity");
        auto invalid = snapshot;
        invalid.id = {};
        require(!commands::restoreEntity(*scene, invalid), "Restore accepted an invalid ID");
        invalid.id = Copy;
        invalid.camera.reset();
        invalid.wasActiveCamera = true;
        require(!commands::restoreEntity(*scene, invalid) && !scene->findEntityById(Copy), "Invalid snapshot left a partial entity");

        require(history.execute(std::make_unique<commands::DeleteEntityCommand>(Source), *scene), "Delete before collision failed");
        const auto collision = scene->createEntityWithId(Source, "Collision");
        require(!history.undo(*scene) && history.canUndo() && !scene->hasComponent<components::TransformComponent>(collision), "Restore collision changed history or entity data");
        scene->destroyEntity(collision);
        require(history.undo(*scene) && !history.isDirty(), "Undo could not recover after resolving a collision");
        history.clear();
        require(history.execute(std::make_unique<commands::DuplicateEntityCommand>(Source, Copy), *scene) && history.undo(*scene), "Duplicate/undo before collision failed");
        const auto copyCollision = scene->createEntityWithId(Copy, "Copy collision");
        require(!history.redo(*scene) && history.canRedo() && !scene->hasComponent<components::TransformComponent>(copyCollision), "Redo overwrote a colliding entity");
        scene->destroyEntity(copyCollision);
        require(history.redo(*scene), "Redo could not recover from an ID collision");
        scene->destroyEntity(*scene->findEntityById(Copy));
        require(!history.undo(*scene) && history.canUndo(), "Missing undo target advanced history");
        require(!scene->isAlive(source), "Deletion failed to invalidate original handle");
    }

    auto orderedHistory() -> void {
        scene_write_tests::Fixture files;
        auto serializer = SceneSerializer::createSceneSerializer(files.scenes);
        auto scene = Scene::createScene("Ordered history");
        commands::CommandHistory history;
        require(history.execute(std::make_unique<commands::CreateEntityCommand>(Source, "Entity"), *scene), "Create failed");
        const AetherEngine::math::Transform before;
        auto after = before;
        after.position = {2, 3, 4};
        require(history.execute(std::make_unique<commands::TransformChangeCommand>(Source, before, after), *scene), "Transform failed");
        require(history.execute(std::make_unique<commands::AddMeshComponentCommand>(Source, components::MeshComponent{.asset = {MeshId}}), *scene), "Mesh failed");
        require(history.execute(std::make_unique<commands::AddMaterialComponentCommand>(Source, components::MaterialComponent{.asset = {MaterialId}}), *scene), "Material failed");
        require(history.execute(std::make_unique<commands::DuplicateEntityCommand>(Source, Copy), *scene), "Duplicate failed");
        require(history.execute(std::make_unique<commands::RenameEntityCommand>(Copy, "Entity Copy", "Clone"), *scene), "Rename failed");
        require(history.execute(std::make_unique<commands::DeleteEntityCommand>(Source), *scene), "Delete source failed");
        const auto expected = sceneJson(*scene, *serializer, files, "Final");
        history.markSaved();
        for (int step = 0; step < 7; ++step) require(history.undo(*scene), "Ordered undo failed");
        require(scene->isEmpty() && history.isDirty(), "Ordered undo did not return to an empty scene");
        for (int step = 0; step < 7; ++step) require(history.redo(*scene), "Ordered redo failed");
        require(!history.isDirty() && sceneJson(*scene, *serializer, files, "Replayed") == expected, "Ordered redo lost data or saved state");
    }
}

int main() {
    int failures = 0;
    failures += scene_write_tests::run("delete, restore all component fields, active camera and save/load", deletionRoundTrip);
    failures += scene_write_tests::run("Ground, identity-only and inactive camera component sets", optionalComponents);
    failures += scene_write_tests::run("duplicate data and asset refs, active camera and source-independent redo", duplicateRoundTrip);
    failures += scene_write_tests::run("unique copy names and UTF-8 length boundaries", duplicateNames);
    failures += scene_write_tests::run("missing targets, invalid snapshots and ID collision guards", guardFailures);
    failures += scene_write_tests::run("creation, transform, assets, duplicate, rename and deletion history", orderedHistory);
    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
