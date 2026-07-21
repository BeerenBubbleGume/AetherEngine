//
// Created by drhaz on 03.07.2026.
//

#ifndef SMB_PHYSICSSYSTEM_HPP
#define SMB_PHYSICSSYSTEM_HPP
#include <memory>
#include <string>
#include <unordered_map>
#include <physx/PxPhysics.h>
#include <physx/PxPhysicsAPI.h>
#include <physx/PxRigidDynamic.h>
#include <entt/entt.hpp>

#include "components/PhysicsComponents.hpp"
#include "components/TransformComponent.hpp"
#include "scene/Scene.hpp"

namespace engine::systems {
    struct PhysicsError {
        int code;
        std::string message;
    };
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
        auto simulate(float fixedDelta) const -> void;
        auto syncSceneFromActors(scene::Scene& scene) -> void;

        auto createMissingActors(scene::Scene& scene) -> void;
        auto createActorForEntity(scene::Scene& scene, scene::Entity entity) -> void;
        auto destroyRemovedActors(scene::Scene& scene) -> void;

        static auto createGeometry(const components::ColliderComponent& collider) -> std::optional<physx::PxGeometryHolder>;

        physx::PxDefaultAllocator m_allocator;
        physx::PxDefaultErrorCallback m_errorCallback;

        physx::PxFoundation* m_foundation{nullptr};
        physx::PxPhysics* m_physics{nullptr};
        physx::PxDefaultCpuDispatcher* m_dispatcher{nullptr};
        physx::PxScene* m_scene{nullptr};
        physx::PxMaterial* m_defaultMaterial{nullptr};

        std::unordered_map<entt::entity, physx::PxRigidActor*> m_actors;

        PhysicsSystem() = default;
        ~PhysicsSystem();
    };
} // systems
// engine

#endif //SMB_PHYSICSSYSTEM_HPP
