//
// Created by drhaz on 08.10.2026.
//

#include "commands/ComponentCommands.hpp"

#include "components/TransformComponent.hpp"
#include "physics/PhysicsValidation.hpp"
#include "scene/Scene.hpp"

namespace {
    template<class Component>
    auto addAssetComponent(AetherEngine::scene::Scene& scene, AetherEngine::scene::EntityId entityId,
                          const Component& value) -> bool {
        const auto entity = scene.findEntityById(entityId);
        if (!entity || !value.asset.isValid() || scene.hasComponent<Component>(*entity)) {
            return false;
        }
        scene.addComponent<Component>(*entity, value);
        return true;
    }

    template<class Component>
    auto removeAssetComponent(AetherEngine::scene::Scene& scene, AetherEngine::scene::EntityId entityId) -> bool {
        const auto entity = scene.findEntityById(entityId);
        if (!entity || !scene.hasComponent<Component>(*entity)) {
            return false;
        }
        scene.removeComponent<Component>(*entity);
        return true;
    }
}

namespace AetherEditor::commands {
    auto AddMeshComponentCommand::apply(AetherEngine::scene::Scene& scene) -> bool {
        return addAssetComponent(scene, m_entityId, m_value);
    }

    auto AddMeshComponentCommand::undo(AetherEngine::scene::Scene& scene) -> bool {
        return removeAssetComponent<AetherEngine::components::MeshComponent>(scene, m_entityId);
    }

    auto AddMaterialComponentCommand::apply(AetherEngine::scene::Scene& scene) -> bool {
        return addAssetComponent(scene, m_entityId, m_value);
    }

    auto AddMaterialComponentCommand::undo(AetherEngine::scene::Scene& scene) -> bool {
        return removeAssetComponent<AetherEngine::components::MaterialComponent>(scene, m_entityId);
    }

    auto TransformChangeCommand::apply(AetherEngine::scene::Scene &scene) -> bool {
        return setTransform(scene, m_after);
    }

    auto TransformChangeCommand::undo(AetherEngine::scene::Scene &scene) -> bool {
        return setTransform(scene, m_before);
    }

    auto TransformChangeCommand::setTransform(AetherEngine::scene::Scene &scene,
        const AetherEngine::math::Transform &value) const -> bool {
        const auto entity = scene.findEntityById(m_entityId);

        if (!entity) {
            return false;
        }

        auto* component = scene.tryGetComponent<AetherEngine::components::TransformComponent>(*entity);

        if (!component) {
            return false;
        }

        component->transform = value;
        return true;
    }

    auto MeshChangeCommand::apply(AetherEngine::scene::Scene &scene) -> bool {
        return setMesh(scene, m_after);
    }

    auto MeshChangeCommand::undo(AetherEngine::scene::Scene &scene) -> bool {
        return setMesh(scene, m_before);
    }

    auto MeshChangeCommand::setMesh(AetherEngine::scene::Scene &scene, const MeshRef &value) const -> bool {
        const auto entity = scene.findEntityById(m_entityId);
        if (!entity) {
            return false;
        }
        auto* component = scene.tryGetComponent<AetherEngine::components::MeshComponent>(*entity);
        if (!component) {
            return false;
        }
        component->asset = value;
        return true;
    }

    auto MaterialChangeCommand::apply(AetherEngine::scene::Scene& scene) -> bool {
        return setMaterial(scene, m_after);
    }

    auto MaterialChangeCommand::undo(AetherEngine::scene::Scene& scene) -> bool {
        return setMaterial(scene, m_before);
    }

    auto MaterialChangeCommand::setMaterial(AetherEngine::scene::Scene& scene, const MaterialRef& value) const -> bool {
        const auto entity = scene.findEntityById(m_entityId);
        if (!entity) {
            return false;
        }
        auto* component = scene.tryGetComponent<AetherEngine::components::MaterialComponent>(*entity);
        if (!component) {
            return false;
        }
        component->asset = value;
        return true;
    }

    auto PhysicsComponentState::capture(AetherEngine::scene::Scene& scene, AetherEngine::scene::Entity entity) -> PhysicsComponentState {
        PhysicsComponentState state;
        if (const auto* body = scene.tryGetComponent<AetherEngine::components::RigidbodyComponent>(entity)) state.rigidbody = *body;
        if (const auto* shape = scene.tryGetComponent<AetherEngine::components::ColliderComponent>(entity)) state.collider = *shape;
        return state;
    }

    auto PhysicsChangeCommand::apply(AetherEngine::scene::Scene &scene) -> bool {
        return setComponents(scene, m_before, m_after);
    }

    auto PhysicsChangeCommand::undo(AetherEngine::scene::Scene &scene) -> bool {
        return setComponents(scene, m_after, m_before);
    }

    auto PhysicsChangeCommand::setComponents(AetherEngine::scene::Scene &scene,
        const PhysicsComponentState& expected, const PhysicsComponentState& value) const -> bool {
        using namespace AetherEngine::components;
        using AetherEngine::physics::invalidPhysicsField;
        const auto entity = scene.findEntityById(m_entityId);
        if (!entity || expected == value || PhysicsComponentState::capture(scene, *entity) != expected ||
            (value.rigidbody && !invalidPhysicsField(*value.rigidbody).empty()) ||
            (value.collider && !invalidPhysicsField(*value.collider).empty())) {
            return false;
        }

        // Perform allocations before overwriting/removing existing components.
        // If the second addition fails, roll back the first one as well.
        bool addedBody = false;
        bool addedCollider = false;
        try {
            if (value.rigidbody && !expected.rigidbody) {
                scene.addComponent<RigidbodyComponent>(*entity, *value.rigidbody);
                addedBody = true;
            }
            if (value.collider && !expected.collider) {
                scene.addComponent<ColliderComponent>(*entity, *value.collider);
                addedCollider = true;
            }
        } catch (...) {
            if (addedBody) scene.removeComponent<RigidbodyComponent>(*entity);
            if (addedCollider) scene.removeComponent<ColliderComponent>(*entity);
            throw;
        }
        if (value.rigidbody) scene.getComponent<RigidbodyComponent>(*entity) = *value.rigidbody;
        else if (expected.rigidbody) scene.removeComponent<RigidbodyComponent>(*entity);
        if (value.collider) scene.getComponent<ColliderComponent>(*entity) = *value.collider;
        else if (expected.collider) scene.removeComponent<ColliderComponent>(*entity);
        return true;
    }
} // commands
// AetherEditor
