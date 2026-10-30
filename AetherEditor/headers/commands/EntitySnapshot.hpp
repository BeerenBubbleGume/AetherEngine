#ifndef SMB_ENTITYSNAPSHOT_HPP
#define SMB_ENTITYSNAPSHOT_HPP

#include <cstdint>
#include <optional>
#include <string>

#include "components/CameraComponent.hpp"
#include "components/MaterialComponent.hpp"
#include "components/MeshComponent.hpp"
#include "components/PhysicsComponents.hpp"
#include "components/TransformComponent.hpp"
#include "scene/EntityId.hpp"

namespace AetherEngine::scene { class Scene; }

namespace AetherEditor::commands {
    struct EntitySnapshot {
        AetherEngine::scene::EntityId id;
        std::string name;
        std::optional<AetherEngine::components::TransformComponent> transform;
        std::optional<AetherEngine::components::MeshComponent> mesh;
        std::optional<AetherEngine::components::MaterialComponent> material;
        std::optional<AetherEngine::components::CameraComponent> camera;
        std::optional<AetherEngine::components::ColliderComponent> collider;
        std::optional<AetherEngine::components::RigidbodyComponent> rigidbody;
        bool wasActiveCamera = false;
    };

    [[nodiscard]] auto captureEntity(AetherEngine::scene::Scene& scene, AetherEngine::scene::EntityId id)
        -> std::optional<EntitySnapshot>;
    [[nodiscard]] auto restoreEntity(AetherEngine::scene::Scene& scene, const EntitySnapshot& snapshot) -> bool;
}

#endif // SMB_ENTITYSNAPSHOT_HPP
