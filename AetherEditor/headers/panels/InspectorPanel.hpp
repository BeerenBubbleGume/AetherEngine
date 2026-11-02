//
// Created by drhaz on 08.10.2026.
//

#ifndef SMB_INSPECTORPANEL_HPP
#define SMB_INSPECTORPANEL_HPP
#include <memory>
#include <imgui.h>
#include <optional>

#include "core/EditorSession.hpp"
#include "commands/CommandHistory.hpp"
#include "commands/ComponentCommands.hpp"
#include "scene/Scene.hpp"
#include "math/Types.hpp"
#include "systems/SceneAssetBindingSystem.hpp"

namespace AetherEngine::assets {
    class AssetRegistry;
}

namespace AetherEngine::components {
    struct MaterialComponent;
    struct MeshComponent;
    struct TransformComponent;
    struct ColliderComponent;
    struct RigidbodyComponent;
}

namespace AetherEditor::panels {
    class InspectorPanel {
    public:
        struct InspectorPanelDeleter {
            void operator()(InspectorPanel* p) const {
                delete p;
            }
        };
        using InspectorPanelPtr = std::unique_ptr<InspectorPanel, InspectorPanelDeleter>;
        static auto createInspectorPanel() -> InspectorPanelPtr;
        auto draw(bool &open, AetherEngine::scene::Scene &scene, const core::EditorSession &session, commands::CommandHistory& history, const
                  AetherEngine::assets::AssetRegistry &registry,
                  std::span<const AetherEngine::systems::SceneAssetBindingError> errors = {}) -> bool;
        auto finishPendingEdits(AetherEngine::scene::Scene& scene, commands::CommandHistory& history) -> void;
        [[nodiscard]] auto hasPendingEdit() const -> bool;
        [[nodiscard]] auto hasPendingChanges(AetherEngine::scene::Scene& scene) const -> bool;
        InspectorPanel(const InspectorPanel&) = delete;
        InspectorPanel& operator=(const InspectorPanel&) = delete;
        InspectorPanel(InspectorPanel&&) = delete;
        InspectorPanel& operator=(InspectorPanel&&) = delete;
    private:
        auto drawEntity(
            AetherEngine::scene::Scene& scene,
            AetherEngine::scene::Entity entity,
            commands::CommandHistory& history,
            const AetherEngine::assets::AssetRegistry& registry,
            bool& editActive
        ) -> bool;
        auto drawTransform(
            AetherEngine::scene::Scene& scene,
            AetherEngine::scene::EntityId entityId,
            AetherEngine::components::TransformComponent& component,
            commands::CommandHistory& history,
            bool& editActive
        ) -> bool;
        auto drawRigidbody(
            AetherEngine::scene::Scene& scene,
            AetherEngine::scene::EntityId entityId,
            AetherEngine::components::RigidbodyComponent& component,
            commands::CommandHistory& history,
            bool& editActive
        ) -> bool;
        auto drawCollider(
            AetherEngine::scene::Scene& scene,
            AetherEngine::scene::EntityId entityId,
            AetherEngine::components::ColliderComponent& component,
            commands::CommandHistory& history,
            bool& editActive
        ) -> bool;
        auto drawPhysicsActions(AetherEngine::scene::Scene& scene, AetherEngine::scene::Entity entity,
                                commands::CommandHistory& history) -> bool;
        auto drawMesh(
            AetherEngine::scene::Scene& scene,
            AetherEngine::scene::Entity entity,
            commands::CommandHistory& history,
            const AetherEngine::assets::AssetRegistry& registry
        ) -> bool;
        auto drawMaterial(
            AetherEngine::scene::Scene& scene,
            AetherEngine::scene::Entity entity,
            commands::CommandHistory& history,
            const AetherEngine::assets::AssetRegistry& registry
        ) -> bool;
        auto beginTransformEdit(
            AetherEngine::scene::Scene& scene,
            AetherEngine::scene::EntityId entityId,
            const AetherEngine::math::Transform& before,
            commands::CommandHistory& history,
            bool& editActive
        ) -> void;
        auto endTransformEdit(AetherEngine::scene::Scene& scene, commands::CommandHistory& history) -> void;
        auto finishTransformEdit(AetherEngine::scene::Scene& scene, commands::CommandHistory& history) -> void;
        auto finishPhysicsEdit(AetherEngine::scene::Scene& scene, commands::CommandHistory& history) -> void;
        auto beginPhysicsEdit(
            AetherEngine::scene::Scene& scene,
            AetherEngine::scene::EntityId entityId,
            const commands::PhysicsComponentState& before,
            commands::CommandHistory& history,
            bool& editActive
        ) -> void;
        auto endPhysicsEdit(AetherEngine::scene::Scene& scene, commands::CommandHistory& history) -> void;
        InspectorPanel() = default;
        ~InspectorPanel() = default;
        struct PendingTransformEdit {
            AetherEngine::scene::EntityId entityId;
            ImGuiID widgetId;
            AetherEngine::math::Transform before;
        };
        struct PendingPhysicsEdit {
            AetherEngine::scene::EntityId entityId;
            ImGuiID widgetId;
            commands::PhysicsComponentState before;
        };

        std::optional<PendingTransformEdit> m_transformEdit;
        std::optional<PendingPhysicsEdit> m_physicsEdit;
        std::optional<AetherEngine::scene::EntityId> m_rotationEntity{};
        AetherEngine::math::Vec3 m_rotationDegrees{};
        AetherEngine::math::Quat m_rotationQuaternion{};
    };
} // panel
// AetherEditor

#endif //SMB_INSPECTORPANEL_HPP
