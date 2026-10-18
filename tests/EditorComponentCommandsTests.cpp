#include "SceneWriteTestSupport.hpp"

#include <cstdlib>
#include <memory>

#include "commands/CommandHistory.hpp"
#include "commands/ComponentCommands.hpp"
#include "commands/EntityCommands.hpp"
#include "components/PhysicsComponents.hpp"
#include "components/TransformComponent.hpp"
#include "systems/SceneSerializer.hpp"

namespace {
    namespace assets = AetherEngine::assets;
    namespace components = AetherEngine::components;
    namespace commands = AetherEditor::commands;
    using scene_write_tests::require;
    using AetherEngine::scene::Scene;

    constexpr assets::AssetId MeshId{1, 1};
    constexpr assets::AssetId MaterialId{2, 2};
    constexpr assets::AssetId OtherMaterialId{3, 3};

    auto renameHistoryAndSave() -> void {
        scene_write_tests::Fixture fixture;
        auto serializer = AetherEngine::systems::SceneSerializer::createSceneSerializer(fixture.scenes);
        auto scene = Scene::createScene("Rename test");
        const AetherEngine::scene::EntityId id{50, 60};
        commands::CommandHistory history;
        require(history.execute(std::make_unique<commands::CreateEntityCommand>(id, "Entity"), *scene), "Create before rename failed");
        const auto entity = *scene->findEntityById(id);
        const auto other = scene->createEntity("Shared");
        history.markSaved();
        const std::string name = "Shared##" "\xd0\xa2\xd0\xb5\xd1\x81\xd1\x82";
        require(history.execute(std::make_unique<commands::RenameEntityCommand>(id, "Entity", name), *scene) && history.isDirty(),
                "Rename did not enter history");
        require(scene->findEntityByName(name).has_value() && scene->findEntityById(id)->handle == entity.handle &&
                scene->getComponent<components::IdentityComponent>(other).name == "Shared", "Rename changed identity or another entity");
        require(serializer->serializeScene(*scene).has_value(), "Could not save renamed entity");
        auto loaded = serializer->deserializeScene("Rename test");
        require(loaded && (*loaded)->getComponent<components::IdentityComponent>(*(*loaded)->findEntityById(id)).name == name,
                "Reload lost the full UTF-8 entity name");
        require(history.undo(*scene) && scene->getComponent<components::IdentityComponent>(entity).name == "Entity" && !history.isDirty(),
                "Undo rename did not restore the previous name and saved state");
        require(history.undo(*scene) && !scene->findEntityById(id), "Undo creation after rename failed");
        require(history.redo(*scene) && history.redo(*scene) &&
                scene->getComponent<components::IdentityComponent>(*scene->findEntityById(id)).name == name,
                "Redo creation and rename did not target the same stable ID");
        require(history.execute(std::make_unique<commands::RenameEntityCommand>(id, name, "Shared"), *scene) &&
                scene->getComponent<components::IdentityComponent>(*scene->findEntityById(id)).name == "Shared",
                "Different entities could not share a display name");
    }

    auto rejectInvalidRename() -> void {
        auto scene = Scene::createScene("Rename guards");
        const auto entity = scene->createEntity("Original");
        const auto id = scene->getComponent<components::IdentityComponent>(entity).id;
        commands::CommandHistory history;
        for (const auto& name : {std::string{}, std::string{" \t\r\n"}, std::string(1025, 'x'), std::string{"Original"},
                                 std::string{"hidden\0suffix", 13}}) {
            require(!history.execute(std::make_unique<commands::RenameEntityCommand>(id, "Original", name), *scene) &&
                    !history.canUndo() && !history.isDirty(), "Invalid or unchanged name entered history");
        }
        require(scene->getComponent<components::IdentityComponent>(entity).name == "Original", "Rejected rename changed the name");
        require(!history.execute(std::make_unique<commands::RenameEntityCommand>(AetherEngine::scene::EntityId{61, 62}, "Missing", "New"), *scene),
                "Rename accepted a missing entity");
        require(history.execute(std::make_unique<commands::RenameEntityCommand>(id, "Original", std::string(1024, 'x')), *scene),
                "Valid maximum-length name was rejected");
        require(history.undo(*scene), "Undo maximum-length rename failed");
        require(!history.execute(std::make_unique<commands::RenameEntityCommand>(id, "Original", "Original"), *scene) && history.canRedo(),
                "Unchanged name discarded redo history");
        require(history.redo(*scene), "Redo after unchanged name failed");
        scene->destroyEntity(entity);
        require(!history.undo(*scene) && history.canUndo(), "Missing rename undo target advanced history");
    }

    auto createUndoRedoAndSave() -> void {
        scene_write_tests::Fixture fixture;
        auto serializer = AetherEngine::systems::SceneSerializer::createSceneSerializer(fixture.scenes);
        auto scene = Scene::createScene("Created entities");
        const auto existing = scene->createEntity("Existing");
        const AetherEngine::scene::EntityId id{10, 20};
        commands::CommandHistory history;
        require(history.execute(std::make_unique<commands::CreateEntityCommand>(id, "Entity"), *scene) && history.isDirty(),
                "Entity creation did not enter history");
        const auto created = scene->findEntityById(id);
        require(created && scene->getComponent<components::IdentityComponent>(*created).name == "Entity",
                "Created entity has no stable identity or name");
        const auto transform = scene->getComponent<components::TransformComponent>(*created).transform;
        require(transform.position == AetherEngine::math::Vec3::zero() &&
                transform.rotation == AetherEngine::math::Quat::identity() &&
                transform.scale == AetherEngine::math::Vec3::one(), "Created entity has a non-default Transform");
        require(!scene->hasComponent<components::MeshComponent>(*created) &&
                !scene->hasComponent<components::MaterialComponent>(*created) &&
                !scene->hasComponent<components::ColliderComponent>(*created), "Empty creation added other components");
        require(serializer->serializeScene(*scene).has_value(), "Could not save created entity");
        auto loaded = serializer->deserializeScene("Created entities");
        require(loaded.has_value(), "Could not reload created entity");
        const auto restored = (*loaded)->findEntityById(id);
        require(restored && (*loaded)->getComponent<components::IdentityComponent>(*restored).name == "Entity" &&
                (*loaded)->hasComponent<components::TransformComponent>(*restored), "Reload lost entity identity or Transform");

        require(history.undo(*scene) && !scene->findEntityById(id) && !history.isDirty() && scene->isAlive(existing),
                "Undo creation changed another entity or failed to return to clean state");
        require(!scene->isAlive(*created), "Undo creation left the old handle alive");
        require(serializer->serializeScene(*scene, "Undone").has_value(), "Could not save undone creation");
        loaded = serializer->deserializeScene("Undone");
        require(loaded && !(*loaded)->findEntityById(id), "Undone entity survived save and reload");
        require(history.redo(*scene) && scene->findEntityById(id).has_value(), "Redo changed the created entity ID");
        require(!scene->isAlive(*created), "Redo resurrected a stale EnTT handle");
        history.markSaved();
        require(history.undo(*scene) && history.isDirty() && history.redo(*scene) && !history.isDirty(),
                "Creation Undo/Redo did not track the saved state");
    }

    auto createdEntityEditHistory() -> void {
        auto scene = Scene::createScene("Creation and editing");
        const AetherEngine::scene::EntityId id{21, 22};
        commands::CommandHistory history;
        require(history.execute(std::make_unique<commands::CreateEntityCommand>(id, "Entity"), *scene), "Create entity failed");
        const AetherEngine::math::Transform before;
        auto after = before;
        after.position = {2.0f, 3.0f, 4.0f};
        require(history.execute(std::make_unique<commands::TransformChangeCommand>(id, before, after), *scene), "Move created entity failed");
        require(history.execute(std::make_unique<commands::AddMeshComponentCommand>(id,
                    components::MeshComponent{.asset = {MeshId}}), *scene), "Assign mesh to created entity failed");
        require(history.execute(std::make_unique<commands::AddMaterialComponentCommand>(id,
                    components::MaterialComponent{.asset = {MaterialId}}), *scene), "Assign material to created entity failed");
        for (int step = 0; step < 4; ++step) require(history.undo(*scene), "Ordered undo after creation failed");
        require(scene->isEmpty() && !history.isDirty(), "Undo did not return to an empty scene");
        for (int step = 0; step < 4; ++step) require(history.redo(*scene), "Ordered redo after creation failed");
        const auto entity = scene->findEntityById(id);
        require(entity && scene->getComponent<components::TransformComponent>(*entity).transform.position == after.position &&
                scene->getComponent<components::MeshComponent>(*entity).asset.id == MeshId &&
                scene->getComponent<components::MaterialComponent>(*entity).asset.id == MaterialId,
                "Redo did not restore subsequent edits on the same entity ID");
        for (int step = 0; step < 4; ++step) require(history.undo(*scene), "Undo before branching failed");
        const AetherEngine::scene::EntityId replacement{23, 24};
        require(history.execute(std::make_unique<commands::CreateEntityCommand>(replacement, "Entity"), *scene) &&
                !history.canRedo() && !scene->findEntityById(id) && scene->findEntityById(replacement),
                "New creation did not replace the redo branch");
    }

    auto rejectInvalidCreation() -> void {
        auto scene = Scene::createScene("Creation guards");
        const AetherEngine::scene::EntityId id{30, 40};
        commands::CommandHistory history;
        require(!history.execute(std::make_unique<commands::CreateEntityCommand>(AetherEngine::scene::EntityId{}, "Entity"), *scene) &&
                scene->isEmpty() && !history.canUndo() && !history.isDirty(), "Zero ID created an entity or history");
        require(history.execute(std::make_unique<commands::CreateEntityCommand>(id, "Entity"), *scene), "Create entity failed");
        require(!history.execute(std::make_unique<commands::CreateEntityCommand>(id, "Duplicate"), *scene) &&
                scene->getComponent<components::IdentityComponent>(*scene->findEntityById(id)).name == "Entity",
                "Duplicate creation overwrote existing identity");
        require(history.undo(*scene) && !history.canUndo(), "Rejected duplicate entered history");
        const auto collision = scene->createEntityWithId(id, "Collision");
        require(!history.redo(*scene) && history.canRedo() && !scene->hasComponent<components::TransformComponent>(collision),
                "Redo collision overwrote a component or advanced history");
        scene->destroyEntity(collision);
        require(history.redo(*scene), "Redo did not recover after resolving the ID collision");
        scene->destroyEntity(*scene->findEntityById(id));
        require(!history.undo(*scene) && history.canUndo(), "Missing undo target advanced history");
    }

    auto checkGround(Scene& scene, AetherEngine::scene::Entity entity) -> void {
        const auto& transform = scene.getComponent<components::TransformComponent>(entity).transform;
        const auto& collider = scene.getComponent<components::ColliderComponent>(entity);
        const auto& body = scene.getComponent<components::RigidbodyComponent>(entity);
        require(transform.position == AetherEngine::math::Vec3{0.0f, -2.0f, 0.0f}, "Ground moved");
        require(collider.type == components::ColliderType::Box &&
                collider.size == AetherEngine::math::Vec3{12.0f, 0.5f, 6.0f}, "Ground collider changed");
        require(!body.dynamic && !body.useGravity && body.mass == 1.0f, "Ground body changed");
    }

    auto addReplaceUndoAndSave() -> void {
        scene_write_tests::Fixture fixture;
        auto serializer = AetherEngine::systems::SceneSerializer::createSceneSerializer(fixture.scenes);
        auto scene = Scene::createScene("Ground test");
        const auto ground = scene->createEntity("Ground");
        const auto groundId = scene->getComponent<components::IdentityComponent>(ground).id;
        scene->addComponent<components::TransformComponent>(ground).transform.position = {0.0f, -2.0f, 0.0f};
        scene->addComponent<components::ColliderComponent>(ground).size = {12.0f, 0.5f, 6.0f};
        scene->addComponent<components::RigidbodyComponent>(ground) = {.dynamic = false, .mass = 1.0f, .useGravity = false};
        const auto untouched = scene->createEntity("Other entity");
        components::MaterialComponent material{.asset = {MaterialId}, .baseColor = {0.2f, 0.4f, 0.6f, 0.8f}};
        commands::CommandHistory history;

        require(history.execute(std::make_unique<commands::AddMaterialComponentCommand>(groundId, material), *scene),
                "Could not add material before mesh");
        require(!scene->hasComponent<components::MeshComponent>(ground), "Adding material also added mesh");
        require(history.execute(std::make_unique<commands::AddMeshComponentCommand>(groundId,
                    components::MeshComponent{.asset = {MeshId}}), *scene), "Could not add mesh");
        require(!scene->hasComponent<components::MeshComponent>(untouched) &&
                !scene->hasComponent<components::MaterialComponent>(untouched), "Wrong entity was modified");

        require(history.execute(std::make_unique<commands::MaterialChangeCommand>(groundId,
                    decltype(material.asset){MaterialId}, decltype(material.asset){OtherMaterialId}), *scene),
                "Could not replace newly added material");
        require(history.undo(*scene) && scene->getComponent<components::MaterialComponent>(ground).asset.id == MaterialId,
                "Undo replacement lost original material");
        require(history.undo(*scene) && !scene->hasComponent<components::MeshComponent>(ground), "Undo did not remove mesh");
        require(history.undo(*scene) && !scene->hasComponent<components::MaterialComponent>(ground) && !history.isDirty(),
                "Undo must remove added material and return to clean state");
        checkGround(*scene, ground);
        require(serializer->serializeScene(*scene, "Without assets").has_value(), "Could not save undone scene");
        auto withoutAssets = serializer->deserializeScene("Without assets");
        require(withoutAssets.has_value(), "Could not reload undone scene");
        const auto bareGround = (*withoutAssets)->findEntityById(groundId);
        require(bareGround && !(*withoutAssets)->hasComponent<components::MeshComponent>(*bareGround) &&
                !(*withoutAssets)->hasComponent<components::MaterialComponent>(*bareGround), "Removed components were saved");
        checkGround(**withoutAssets, *bareGround);

        require(history.redo(*scene) && history.redo(*scene), "Redo component additions failed");
        const auto& restored = scene->getComponent<components::MaterialComponent>(ground);
        require(restored.asset.id == MaterialId && restored.baseColor.r == 0.2f && restored.baseColor.g == 0.4f &&
                restored.baseColor.b == 0.6f && restored.baseColor.a == 0.8f, "Redo lost full material component");
        require(serializer->serializeScene(*scene, "With assets").has_value(), "Could not save assigned resources");
        history.markSaved();
        auto loaded = serializer->deserializeScene("With assets");
        require(loaded.has_value(), "Could not reload assigned resources");
        const auto loadedGround = (*loaded)->findEntityById(groundId);
        require(loadedGround.has_value(), "Reload lost Ground identity");
        require((*loaded)->getComponent<components::MeshComponent>(*loadedGround).asset.id == MeshId &&
                (*loaded)->getComponent<components::MaterialComponent>(*loadedGround).asset.id == MaterialId &&
                (*loaded)->getComponent<components::MaterialComponent>(*loadedGround).baseColor.a == 0.8f,
                "Reload lost assigned components");
        checkGround(**loaded, *loadedGround);
        require(history.undo(*scene) && history.isDirty() && history.redo(*scene) && !history.isDirty(),
                "Undo/redo did not track the saved state");
    }

    template<class Component, class AddCommand>
    auto rejectsInvalidAdditions(assets::AssetId id) -> void {
        auto scene = Scene::createScene("Command guards");
        const auto entity = scene->createEntity("Target");
        const auto entityId = scene->getComponent<components::IdentityComponent>(entity).id;
        commands::CommandHistory history;
        const Component valid{.asset = {id}};
        require(!history.execute(std::make_unique<AddCommand>(entityId, Component{}), *scene) &&
                !history.canUndo() && !history.isDirty() && !scene->hasComponent<Component>(entity),
                "Empty asset reference created a component or history");
        require(!history.execute(std::make_unique<AddCommand>(AetherEngine::scene::EntityId{42, 43}, valid), *scene),
                "Missing entity was accepted");
        require(history.execute(std::make_unique<AddCommand>(entityId, valid), *scene), "Valid addition failed");
        require(!history.execute(std::make_unique<AddCommand>(entityId, Component{.asset = {{9, 9}}}), *scene) &&
                scene->getComponent<Component>(entity).asset.id == id, "Addition overwrote an existing component");
        require(history.undo(*scene) && !history.canUndo() && !history.isDirty(), "Rejected addition entered history");
        require(!history.execute(std::make_unique<AddCommand>(entityId, Component{}), *scene) && history.canRedo(),
                "Rejected addition discarded redo history");
        require(history.redo(*scene), "Redo failed after rejected addition");
        scene->destroyEntity(entity);
        require(!history.undo(*scene) && history.canUndo(), "Missing undo target advanced history");
    }
}

int main() {
    int failures = 0;
    failures += scene_write_tests::run("entity rename, stable identity, ordered history and scene round trip", renameHistoryAndSave);
    failures += scene_write_tests::run("invalid and unchanged entity names do not enter history", rejectInvalidRename);
    failures += scene_write_tests::run("entity creation, stable IDs, Undo/Redo and scene round trip", createUndoRedoAndSave);
    failures += scene_write_tests::run("entity creation followed by Transform and resource history", createdEntityEditHistory);
    failures += scene_write_tests::run("entity creation guards and history failures", rejectInvalidCreation);
    failures += scene_write_tests::run("asset components, ordered history, Ground preservation and scene round trip", addReplaceUndoAndSave);
    failures += scene_write_tests::run("mesh addition guards", [] {
        rejectsInvalidAdditions<components::MeshComponent, commands::AddMeshComponentCommand>(MeshId);
    });
    failures += scene_write_tests::run("material addition guards", [] {
        rejectsInvalidAdditions<components::MaterialComponent, commands::AddMaterialComponentCommand>(MaterialId);
    });
    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
