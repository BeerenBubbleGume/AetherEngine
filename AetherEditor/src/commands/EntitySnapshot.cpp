#include "commands/EntitySnapshot.hpp"

#include "scene/Scene.hpp"

namespace {
    template<class Component>
    auto captureComponent(AetherEngine::scene::Scene& scene, AetherEngine::scene::Entity entity)
        -> std::optional<Component> {
        if (const auto* component = scene.tryGetComponent<Component>(entity)) {
            return *component;
        }
        return std::nullopt;
    }

    template<class Component>
    auto restoreComponent(AetherEngine::scene::Scene& scene, AetherEngine::scene::Entity entity,
                          const std::optional<Component>& value) -> void {
        if (value) {
            scene.addComponent<Component>(entity, *value);
        }
    }
}

namespace AetherEditor::commands {
    auto captureEntity(AetherEngine::scene::Scene& scene, AetherEngine::scene::EntityId id)
        -> std::optional<EntitySnapshot> {
        const auto entity = scene.findEntityById(id);
        if (!entity) {
            return std::nullopt;
        }
        const auto& identity = scene.getComponent<AetherEngine::components::IdentityComponent>(*entity);
        const auto activeCamera = scene.getActiveCamera();
        return EntitySnapshot{
            .id = identity.id,
            .name = identity.name,
            .transform = captureComponent<AetherEngine::components::TransformComponent>(scene, *entity),
            .mesh = captureComponent<AetherEngine::components::MeshComponent>(scene, *entity),
            .material = captureComponent<AetherEngine::components::MaterialComponent>(scene, *entity),
            .camera = captureComponent<AetherEngine::components::CameraComponent>(scene, *entity),
            .collider = captureComponent<AetherEngine::components::ColliderComponent>(scene, *entity),
            .rigidbody = captureComponent<AetherEngine::components::RigidbodyComponent>(scene, *entity),
            .wasActiveCamera = activeCamera.scene == &scene && activeCamera.handle == entity->handle
        };
    }

    auto restoreEntity(AetherEngine::scene::Scene& scene, const EntitySnapshot& snapshot) -> bool {
        if (!snapshot.id.isValid() || scene.findEntityById(snapshot.id) ||
            (snapshot.wasActiveCamera && !snapshot.camera)) {
            return false;
        }
        const auto entity = scene.createEntityWithId(snapshot.id, snapshot.name);
        try {
            restoreComponent(scene, entity, snapshot.transform);
            restoreComponent(scene, entity, snapshot.mesh);
            restoreComponent(scene, entity, snapshot.material);
            restoreComponent(scene, entity, snapshot.camera);
            restoreComponent(scene, entity, snapshot.collider);
            restoreComponent(scene, entity, snapshot.rigidbody);
            if (snapshot.wasActiveCamera) {
                scene.setActiveCamera(entity);
            }
        } catch (...) {
            scene.destroyEntity(entity);
            throw;
        }
        return true;
    }
}
