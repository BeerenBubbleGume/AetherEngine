#include "scene/Scene.hpp"

#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_set>

using AetherEngine::scene::Entity;
using AetherEngine::scene::EntityId;
using AetherEngine::scene::EntityIdHash;
using AetherEngine::scene::Scene;
using AetherEngine::components::CameraComponent;
using AetherEngine::components::IdentityComponent;

namespace {
struct Marker { int value = 7; };

void require(bool condition, std::string_view message) {
    if (!condition) {
        throw std::runtime_error(std::string(message));
    }
}

template<class Function>
void rejects(Function operation, std::string_view message) {
    bool rejected = false;
    try {
        operation();
    } catch (const std::runtime_error&) {
        rejected = true;
    }
    require(rejected, message);
}

bool sameEntity(Entity left, Entity right) {
    return left.scene == right.scene && left.handle == right.handle;
}

void rejectsAccess(Scene& scene, Entity invalid) {
    const Scene& constScene = scene;
    require(!constScene.isAlive(invalid), "Invalid or foreign entity is considered alive");
    rejects([&] { scene.addComponent<Marker>(invalid); }, "addComponent accepted invalid entity");
    rejects([&] { scene.getComponent<Marker>(invalid); }, "getComponent accepted invalid entity");
    rejects([&] { scene.removeComponent<Marker>(invalid); }, "removeComponent accepted invalid entity");
    rejects([&] { (void)constScene.hasComponent<Marker>(invalid); }, "hasComponent accepted invalid entity");
    rejects([&] { (void)scene.tryGetComponent<Marker>(invalid); }, "tryGetComponent accepted invalid entity");
    rejects([&] { scene.setActiveCamera(invalid); }, "setActiveCamera accepted invalid entity");
    rejects([&] { scene.destroyEntity(invalid); }, "destroyEntity accepted invalid entity");
}

void creationAndLookup() {
    auto scene = Scene::createScene("Entities");
    const Scene& constScene = *scene;
    std::unordered_set<EntityId, EntityIdHash> ids;
    for (int index = 0; index < 64; ++index) {
        const auto entity = scene->createEntity("Shared name");
        require(entity.isValid() && constScene.isAlive(entity), "Created entity is not alive");
        require(constScene.hasComponent<IdentityComponent>(entity), "Creation did not assign identity");
        const auto identity = scene->getComponent<IdentityComponent>(entity);
        require(identity.id.isValid(), "Generated ID is zero");
        require(ids.insert(identity.id).second, "Creation reused an ID");
        require(identity.name == "Shared name", "Creation lost the display name");
        const auto found = scene->findEntityById(identity.id);
        require(found && sameEntity(*found, entity), "ID lookup returned another entity");
        scene->getComponent<IdentityComponent>(entity).name = "Renamed";
        const auto renamed = scene->findEntityById(identity.id);
        require(renamed && sameEntity(*renamed, entity), "Renaming changed ID lookup");
        require(scene->tryGetComponent<Marker>(entity) == nullptr, "Missing component must return nullptr");
        scene->addComponent<Marker>(entity);
        require(scene->getComponent<Marker>(entity).value == 7, "Component was not added");
        scene->removeComponent<Marker>(entity);
        require(!constScene.hasComponent<Marker>(entity), "Component was not removed");
    }
    require(!scene->findEntityById(EntityId{}), "Missing ID unexpectedly resolves");
}

void duplicateId() {
    auto scene = Scene::createScene("Duplicate IDs");
    const EntityId id{0x1234, 0x5678};
    const auto original = scene->createEntityWithId(id, "Original");
    rejects([&] { scene->createEntityWithId(id, "Duplicate"); }, "Duplicate ID was accepted");
    const auto found = scene->findEntityById(id);
    require(found && sameEntity(*found, original), "Rejected duplicate replaced the original lookup");
    require(scene->view<IdentityComponent>().size() == 1, "Rejected creation left an extra identity");
    require(scene->getComponent<IdentityComponent>(original).name == "Original", "Duplicate changed original data");
    scene->destroyEntity(original);
    const auto restored = scene->createEntityWithId(id, "Restored");
    require(restored.isValid(), "Destroyed ID could not be restored for Undo");
    require(!original.isValid(), "Restoring the ID resurrected the stale handle");
    const auto restoredLookup = scene->findEntityById(id);
    require(restoredLookup && sameEntity(*restoredLookup, restored), "Restored ID lookup is incorrect");
    scene->destroyEntity(restored);
    const auto live = scene->view<entt::entity>();
    require(live.begin() == live.end(), "Rejected duplicate left a hidden entity behind");
}

void emptyAfterDeletion() {
    auto scene = Scene::createScene("Empty scene");
    require(scene->isEmpty(), "New scene is not empty");
    const auto first = scene->createEntity("First");
    const auto second = scene->createEntity("Second");
    require(!scene->isEmpty(), "Scene with live entities is empty");
    scene->destroyEntity(first);
    require(!scene->isEmpty(), "Scene with one remaining entity is empty");
    scene->destroyEntity(second);
    const auto live = scene->view<entt::entity>();
    require(live.begin() == live.end(), "Deleted scene still contains live entities");
    require(scene->isEmpty(), "isEmpty is false after deleting every entity");
    const auto reused = scene->createEntity("Reused slot");
    require(!scene->isEmpty(), "Repopulated scene is empty");
    scene->destroyEntity(reused);
    require(scene->isEmpty(), "isEmpty is false after deleting a reused slot");
}

void zeroId() {
    auto scene = Scene::createScene("Zero ID");
    rejects([&] { scene->createEntityWithId(EntityId{}, "Invalid"); }, "createEntityWithId accepted a zero ID");
    require(scene->isEmpty(), "Rejected zero ID left an entity behind");
    require(!scene->findEntityById(EntityId{}), "Rejected zero ID was indexed");
}

void foreignAndNullEntities() {
    auto scene = Scene::createScene("Local");
    auto other = Scene::createScene("Other");
    const EntityId id{1, 42};
    const auto local = scene->createEntityWithId(id, "Local");
    const auto foreign = other->createEntityWithId(id, "Other");
    require(local.handle == foreign.handle, "Fixture must exercise matching handles in different registries");
    scene->addComponent<Marker>(local);
    scene->addComponent<CameraComponent>(local);
    scene->setActiveCamera(local);
    other->addComponent<Marker>(foreign);
    other->addComponent<CameraComponent>(foreign);
    rejectsAccess(*scene, foreign);
    rejectsAccess(*scene, Entity{});
    rejectsAccess(*scene, Entity{.handle = entt::null, .scene = scene.get()});
    rejectsAccess(*scene, Entity{.handle = local.handle, .scene = nullptr});
    require(foreign.isValid() && local.isValid(), "Invalid operation destroyed a valid entity");
    require(scene->getComponent<Marker>(local).value == 7, "Foreign operation changed the local component");
    require(other->getComponent<Marker>(foreign).value == 7, "Foreign operation changed the other scene");
    require(sameEntity(scene->getActiveCamera(), local), "Rejected operation changed active camera");
}

void staleGeneration() {
    auto scene = Scene::createScene("Generation");
    const auto old = scene->createEntity("Old");
    const auto oldId = scene->getComponent<IdentityComponent>(old).id;
    scene->destroyEntity(old);
    require(!old.isValid(), "Deleted entity is still valid");
    require(!scene->findEntityById(oldId), "Deleted ID is still indexed");
    const auto replacement = scene->createEntity("Replacement");
    require(entt::to_entity(old.handle) == entt::to_entity(replacement.handle), "Fixture did not reuse the freed slot");
    require(old.handle != replacement.handle, "Reused slot did not change generation");
    scene->addComponent<Marker>(replacement);
    rejectsAccess(*scene, old);
    require(replacement.isValid(), "Stale operation destroyed the replacement");
    require(scene->getComponent<Marker>(replacement).value == 7, "Stale operation changed the replacement");
}

void identityProtection() {
    auto scene = Scene::createScene("Identity protection");
    const auto entity = scene->createEntity("Camera");
    const auto id = scene->getComponent<IdentityComponent>(entity).id;
    scene->addComponent<Marker>(entity);
    scene->addComponent<CameraComponent>(entity);
    scene->setActiveCamera(entity);
    rejects([&] { scene->removeComponent<IdentityComponent>(entity); }, "Mandatory identity was removed");
    // The forbidden type may occur anywhere in a batch: reject before removing anything.
    rejects([&] { scene->removeComponent<Marker, CameraComponent, IdentityComponent>(entity); },
            "Batch removal accepted mandatory identity");
    require(scene->hasComponent<Marker, CameraComponent, IdentityComponent>(entity), "Rejected batch partially removed components");
    require(sameEntity(scene->getActiveCamera(), entity), "Rejected batch cleared active camera");
    require(scene->getComponent<IdentityComponent>(entity).id == id, "Rejected removal changed ID");
    const auto found = scene->findEntityById(id);
    require(found && sameEntity(*found, entity), "Rejected removal damaged the ID index");
}

void cameraValidation() {
    auto scene = Scene::createScene("Camera validation");
    require(!scene->getActiveCamera().isValid(), "Empty scene has an active camera");
    const auto camera = scene->createEntity("Camera");
    scene->addComponent<CameraComponent>(camera);
    scene->setActiveCamera(camera);
    const auto object = scene->createEntity("No camera component");
    rejects([&] { scene->setActiveCamera(object); }, "Entity without camera component was accepted");
    require(sameEntity(scene->getActiveCamera(), camera), "Rejected camera selection changed the current camera");
    scene->destroyEntity(object);
    require(sameEntity(scene->getActiveCamera(), camera), "Deleting an unrelated entity cleared the camera");
}

void cameraDeletion() {
    auto scene = Scene::createScene("Camera deletion");
    const auto active = scene->createEntity("Active");
    const auto inactive = scene->createEntity("Inactive");
    scene->addComponent<CameraComponent>(active);
    scene->addComponent<CameraComponent>(inactive);
    scene->setActiveCamera(active);
    scene->destroyEntity(inactive);
    require(sameEntity(scene->getActiveCamera(), active), "Deleting an inactive camera cleared the active camera");
    const auto id = scene->getComponent<IdentityComponent>(active).id;
    scene->destroyEntity(active);
    const auto empty = scene->getActiveCamera();
    require(empty.handle == entt::null && empty.scene == nullptr, "Deleting active camera left a stale reference");
    require(!scene->findEntityById(id), "Deleting active camera left its ID indexed");
}

void cameraComponentRemoval() {
    auto scene = Scene::createScene("Camera component removal");
    const auto active = scene->createEntity("Active");
    const auto inactive = scene->createEntity("Inactive");
    scene->addComponent<CameraComponent>(active);
    scene->addComponent<CameraComponent>(inactive);
    scene->addComponent<Marker>(active);
    scene->setActiveCamera(active);
    scene->removeComponent<CameraComponent>(inactive);
    require(sameEntity(scene->getActiveCamera(), active), "Removing an inactive camera cleared the active camera");
    scene->removeComponent<Marker>(active);
    require(sameEntity(scene->getActiveCamera(), active), "Removing another component cleared the camera");
    scene->removeComponent<CameraComponent>(active);
    auto empty = scene->getActiveCamera();
    require(empty.handle == entt::null && empty.scene == nullptr, "Removing CameraComponent left a stale active camera");
    require(active.isValid() && scene->hasComponent<IdentityComponent>(active), "Removing CameraComponent destroyed the entity");
    scene->addComponent<CameraComponent>(active);
    scene->addComponent<Marker>(active);
    scene->setActiveCamera(active);
    scene->removeComponent<Marker, CameraComponent>(active);
    empty = scene->getActiveCamera();
    require(empty.handle == entt::null && empty.scene == nullptr, "Batch removal left a stale active camera");
    require(!scene->hasComponent<CameraComponent>(active) && !scene->hasComponent<Marker>(active), "Batch removal left components");
}
}

int main(int argc, char** argv) {
    const std::pair<std::string_view, void(*)()> scenarios[] = {
        {"creation", creationAndLookup}, {"duplicate_id", duplicateId}, {"zero_id", zeroId},
        {"foreign_entities", foreignAndNullEntities}, {"stale_generation", staleGeneration},
        {"empty_after_deletion", emptyAfterDeletion},
        {"identity_protection", identityProtection}, {"camera_validation", cameraValidation},
        {"camera_deletion", cameraDeletion}, {"camera_component_removal", cameraComponentRemoval}
    };
    if (argc == 2) {
        for (const auto& [name, function] : scenarios) {
            if (name == argv[1]) {
                try {
                    function();
                    std::cout << "PASS: " << name << '\n';
                    return 0;
                } catch (const std::exception& error) {
                    std::cerr << "FAIL: " << name << ": " << error.what() << '\n';
                    return 1;
                }
            }
        }
    }
    std::cerr << "Expected a known scene entity scenario name\n";
    return 2;
}
