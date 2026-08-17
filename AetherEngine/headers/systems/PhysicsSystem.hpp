//
// Created by drhaz on 03.07.2026.
//

#ifndef SMB_PHYSICSSYSTEM_HPP
#define SMB_PHYSICSSYSTEM_HPP
#include <expected>
#include <memory>
#include <unordered_map>

#include <entt/entt.hpp>

#include "components/PhysicsComponents.hpp"
#include "components/TransformComponent.hpp"
#include "physics/PhysicsTypes.hpp"
#include "scene/Scene.hpp"

namespace AetherEngine::physics {
    class IPhysicsBackend;
}

namespace AetherEngine::systems {
    using PhysicsError = physics::PhysicsError;

    class PhysicsSystem final {
    public:
        struct PhysicsSystemDeleter {
            void operator()(PhysicsSystem* system) const;
        };
        using PhysicsSystemPtr = std::unique_ptr<PhysicsSystem, PhysicsSystemDeleter>;

        [[nodiscard]] static PhysicsSystemPtr createPhysicsSystem();
        [[nodiscard]] auto init() -> std::expected<void, PhysicsError>;
        auto shutdown() -> void;

        auto fixedUpdate(scene::Scene& scene, float fixedDelta) -> void;

    private:
        auto syncActorsFromScene(scene::Scene& scene) -> void;
        auto syncSceneFromActors(scene::Scene& scene) -> void;

        auto createMissingActors(scene::Scene& scene) -> void;
        auto createActorForEntity(scene::Scene& scene, scene::Entity entity) -> void;
        auto destroyRemovedActors(scene::Scene& scene) -> void;

        std::unique_ptr<physics::IPhysicsBackend> m_backend;
        std::unordered_map<entt::entity, physics::BodyHandle> m_actors;

        explicit PhysicsSystem(std::unique_ptr<physics::IPhysicsBackend> backend);
        ~PhysicsSystem();
    };
} // systems
// AetherEngine

#endif //SMB_PHYSICSSYSTEM_HPP
