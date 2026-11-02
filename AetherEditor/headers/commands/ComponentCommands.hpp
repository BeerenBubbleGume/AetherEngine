//
// Created by drhaz on 08.10.2026.
//

#ifndef AETHER_EDITOR_COMPONENTCOMMANDS_HPP
#define AETHER_EDITOR_COMPONENTCOMMANDS_HPP
#include <utility>
#include <optional>

#include "EditorCommand.hpp"
#include "components/MaterialComponent.hpp"
#include "components/MeshComponent.hpp"
#include "math/Types.hpp"
#include "components/PhysicsComponents.hpp"
#include "scene/Entity.hpp"
#include "scene/EntityId.hpp"

namespace AetherEditor::commands {
    class AddMeshComponentCommand final : public EditorCommand {
    public:
        AddMeshComponentCommand(AetherEngine::scene::EntityId entityId, AetherEngine::components::MeshComponent value)
            : m_entityId(entityId), m_value(std::move(value)) {}

        auto apply(AetherEngine::scene::Scene& scene) -> bool override;
        auto undo(AetherEngine::scene::Scene& scene) -> bool override;

    private:
        AetherEngine::scene::EntityId m_entityId;
        AetherEngine::components::MeshComponent m_value;
    };

    class AddMaterialComponentCommand final : public EditorCommand {
    public:
        AddMaterialComponentCommand(AetherEngine::scene::EntityId entityId, AetherEngine::components::MaterialComponent value)
            : m_entityId(entityId), m_value(std::move(value)) {}

        auto apply(AetherEngine::scene::Scene& scene) -> bool override;
        auto undo(AetherEngine::scene::Scene& scene) -> bool override;

    private:
        AetherEngine::scene::EntityId m_entityId;
        AetherEngine::components::MaterialComponent m_value;
    };

    class TransformChangeCommand final : public EditorCommand {
    public:
        TransformChangeCommand(AetherEngine::scene::EntityId entityId, AetherEngine::math::Transform before, AetherEngine::math::Transform after)
            : m_entityId(entityId),
              m_before(std::move(before)),
              m_after(std::move(after)) {}

        auto apply(AetherEngine::scene::Scene& scene) -> bool override;
        auto undo(AetherEngine::scene::Scene& scene) -> bool override;

    private:
        auto setTransform(
            AetherEngine::scene::Scene& scene,
            const AetherEngine::math::Transform& value
        ) const -> bool;

        AetherEngine::scene::EntityId m_entityId;
        AetherEngine::math::Transform m_before;
        AetherEngine::math::Transform m_after;
    };

    class MeshChangeCommand final : public EditorCommand {
        using MeshRef = AetherEngine::assets::AssetRef<AetherEngine::assets::RuntimeMeshAsset>;
    public:
        MeshChangeCommand(AetherEngine::scene::EntityId entityId, MeshRef before, MeshRef after)
            : m_entityId(entityId),
              m_before(std::move(before)),
              m_after(std::move(after)) {};
        auto apply(AetherEngine::scene::Scene& scene) -> bool override;
        auto undo(AetherEngine::scene::Scene& scene) -> bool override;
    private:
        auto setMesh(AetherEngine::scene::Scene& scene, const MeshRef& value) const -> bool;

        AetherEngine::scene::EntityId m_entityId;
        MeshRef m_before;
        MeshRef m_after;
    };

    class MaterialChangeCommand final : public EditorCommand {
        using MaterialRef = AetherEngine::assets::AssetRef<AetherEngine::assets::RuntimeMaterialAsset>;
    public:
        MaterialChangeCommand(AetherEngine::scene::EntityId entityId, MaterialRef before, MaterialRef after)
            : m_entityId(entityId), m_before(before), m_after(after) {}

        auto apply(AetherEngine::scene::Scene& scene) -> bool override;
        auto undo(AetherEngine::scene::Scene& scene) -> bool override;

    private:
        auto setMaterial(AetherEngine::scene::Scene& scene, const MaterialRef& value) const -> bool;

        AetherEngine::scene::EntityId m_entityId;
        MaterialRef m_before;
        MaterialRef m_after;
    };

    struct PhysicsComponentState {
        std::optional<AetherEngine::components::RigidbodyComponent> rigidbody;
        std::optional<AetherEngine::components::ColliderComponent> collider;

        static auto capture(AetherEngine::scene::Scene& scene, AetherEngine::scene::Entity entity) -> PhysicsComponentState;
        auto operator==(const PhysicsComponentState&) const -> bool = default;
    };

    class PhysicsChangeCommand final : public EditorCommand {
    public:
        PhysicsChangeCommand(AetherEngine::scene::EntityId entityId, PhysicsComponentState before, PhysicsComponentState after)
            : m_entityId(entityId), m_before(std::move(before)), m_after(std::move(after)) {}

        auto apply(AetherEngine::scene::Scene &scene) -> bool override;
        auto undo(AetherEngine::scene::Scene &scene) -> bool override;

    private:
        auto setComponents(
            AetherEngine::scene::Scene& scene,
            const PhysicsComponentState& expected,
            const PhysicsComponentState& value
        ) const -> bool;
        AetherEngine::scene::EntityId m_entityId;
        PhysicsComponentState m_before;
        PhysicsComponentState m_after;
    };
} // commands
// AetherEditor

#endif //AETHER_EDITOR_COMPONENTCOMMANDS_HPP
