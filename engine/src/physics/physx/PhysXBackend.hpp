//
// Created by drhaz on 23.07.2026.
//

#ifndef SMB_PHYSXBACKEND_HPP
#define SMB_PHYSXBACKEND_HPP
#include <cstdint>
#include <optional>
#include <unordered_map>

#include <physx/PxPhysicsAPI.h>

#include "../IPhysicsBackend.hpp"


namespace engine::physics {
    class PhysXBackend final : public IPhysicsBackend {
    public:
        PhysXBackend() = default;
        ~PhysXBackend() override;

        auto init() -> std::expected<void, PhysicsError> override;
        auto shutdown() -> void override;

        auto setTransform(BodyHandle handle, const math::Transform &transform) -> void override;
        auto simulate(float fixedDelta) -> void override;

        [[nodiscard]] auto createBody(const BodyDesc& desc) -> BodyHandle override;
        auto destroyBody(BodyHandle handle) -> void override;
        [[nodiscard]] auto getTransform(BodyHandle handle) const -> math::Transform override;

    private:
        [[nodiscard]] static auto createGeometry(
            const components::ColliderComponent& collider
        ) -> std::optional<physx::PxGeometryHolder>;

        [[nodiscard]] auto allocateBodyHandle() -> BodyHandle;

        physx::PxDefaultAllocator m_allocator;
        physx::PxDefaultErrorCallback m_errorCallback;

        physx::PxFoundation* m_foundation{nullptr};
        physx::PxPhysics* m_physics{nullptr};
        physx::PxDefaultCpuDispatcher* m_dispatcher{nullptr};
        physx::PxScene* m_scene{nullptr};
        physx::PxMaterial* m_defaultMaterial{nullptr};

        std::unordered_map<std::uint32_t, physx::PxRigidActor*> m_bodies;
        std::uint32_t m_nextBodyHandle{1};
    };
} // physics
// engine

#endif //SMB_PHYSXBACKEND_HPP
