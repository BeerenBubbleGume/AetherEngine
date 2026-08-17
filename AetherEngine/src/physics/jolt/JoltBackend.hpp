//
// Created by Codex on 04.08.2026.
//

#ifndef SMB_JOLTBACKEND_HPP
#define SMB_JOLTBACKEND_HPP

#include <cstdint>
#include <memory>
#include <unordered_map>

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/BodyID.h>
#include <Jolt/Physics/Collision/Shape/Shape.h>

#include "../IPhysicsBackend.hpp"

namespace JPH {
    class JobSystemThreadPool;
    class PhysicsSystem;
    class TempAllocatorImpl;
}

namespace AetherEngine::physics {
    class JoltBackend final : public IPhysicsBackend {
    public:
        JoltBackend();
        ~JoltBackend() override;

        auto init() -> std::expected<void, PhysicsError> override;
        auto shutdown() -> void override;

        [[nodiscard]] auto createBody(const BodyDesc& desc) -> BodyHandle override;
        auto destroyBody(BodyHandle handle) -> void override;

        auto setTransform(
            BodyHandle handle,
            const math::Transform& transform
        ) -> void override;
        [[nodiscard]] auto getTransform(BodyHandle handle) const -> math::Transform override;

        auto simulate(float fixedDelta) -> void override;

    private:
        class BroadPhaseLayerInterface;
        class ObjectVsBroadPhaseLayerFilter;
        class ObjectLayerPairFilter;

        [[nodiscard]] static auto createShape(
            const components::ColliderComponent& collider
        ) -> JPH::ShapeRefC;

        [[nodiscard]] auto allocateBodyHandle() -> BodyHandle;

        std::unique_ptr<BroadPhaseLayerInterface> m_broadPhaseLayerInterface;
        std::unique_ptr<ObjectVsBroadPhaseLayerFilter> m_objectVsBroadPhaseLayerFilter;
        std::unique_ptr<ObjectLayerPairFilter> m_objectLayerPairFilter;
        std::unique_ptr<JPH::PhysicsSystem> m_physicsSystem;
        std::unique_ptr<JPH::TempAllocatorImpl> m_tempAllocator;
        std::unique_ptr<JPH::JobSystemThreadPool> m_jobSystem;

        std::unordered_map<std::uint32_t, JPH::BodyID> m_bodies;
        std::uint32_t m_nextBodyHandle{1};
        bool m_ownsFactory{false};
    };
}

#endif // SMB_JOLTBACKEND_HPP
