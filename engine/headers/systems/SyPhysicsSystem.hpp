//
// Created by drhaz on 03.07.2026.
//

#ifndef SMB_SYPHYSICSSYSTEM_HPP
#define SMB_SYPHYSICSSYSTEM_HPP
#include <memory>
#include <string>
#include <unordered_map>
#include <physx/PxPhysics.h>
#include <physx/PxPhysicsAPI.h>
#include <physx/PxRigidDynamic.h>
#include <entt/entt.hpp>

#include "components/IPhysicsComponents.hpp"
#include "components/ITransformComponent.hpp"
#include "scene/SCScene.hpp"

namespace engine::systems {
    struct SYPhysicsError {
        int code;
        std::string message;
    };
    class SYPhysicsSystem final {
    public:
        struct SYPhysicsSystemDeleter {
            void operator()(SYPhysicsSystem* system) const;
        };
        using SYPhysicsSystemPtr = std::unique_ptr<SYPhysicsSystem, SYPhysicsSystemDeleter>;

        [[nodiscard]] static SYPhysicsSystemPtr createPhysicsSystem();
        [[nodiscard]] auto init() -> std::expected<void, SYPhysicsError>;
        auto shutdown() -> void;

        auto fixedUpdate(scene::SCScene& scene, float fixedDelta) -> void;
    private:
        auto syncActorsFromScene(scene::SCScene& scene) -> void;
        auto simulate(float fixedDelta) const -> void;
        auto syncSceneFromActors(scene::SCScene& scene) -> void;

        auto createMissingActors(scene::SCScene& scene) -> void;
        auto createActorForEntity(scene::SCScene& scene, scene::Entity entity) -> void;
        auto destroyRemovedActors(scene::SCScene& scene) -> void;

        static auto createGeometry(const components::IColliderComponent& collider) -> std::optional<physx::PxGeometryHolder>;

        physx::PxDefaultAllocator m_allocator;
        physx::PxDefaultErrorCallback m_errorCallback;

        physx::PxFoundation* m_foundation{nullptr};
        physx::PxPhysics* m_physics{nullptr};
        physx::PxDefaultCpuDispatcher* m_dispatcher{nullptr};
        physx::PxScene* m_scene{nullptr};
        physx::PxMaterial* m_defaultMaterial{nullptr};

        std::unordered_map<entt::entity, physx::PxRigidActor*> m_actors;

        SYPhysicsSystem() = default;
        ~SYPhysicsSystem();
    };
} // systems
// engine

#endif //SMB_SYPHYSICSSYSTEM_HPP
