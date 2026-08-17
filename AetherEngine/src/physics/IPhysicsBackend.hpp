//
// Created by drhaz on 23.07.2026.
//

#ifndef SMB_IPHYSICSBACKEND_HPP
#define SMB_IPHYSICSBACKEND_HPP
#include <expected>

#include "physics/PhysicsTypes.hpp"


namespace AetherEngine::physics {
    class IPhysicsBackend {
    public:
        virtual ~IPhysicsBackend() = default;

        virtual auto init() -> std::expected<void, PhysicsError> = 0;
        virtual auto shutdown() -> void = 0;

        [[nodiscard]] virtual auto createBody(const BodyDesc& desc) -> BodyHandle = 0;
        virtual auto destroyBody(BodyHandle handle) -> void = 0;

        virtual auto setTransform(
            BodyHandle handle,
            const math::Transform& transform
        ) -> void = 0;

        [[nodiscard]] virtual auto getTransform(BodyHandle handle) const -> math::Transform = 0;

        virtual auto simulate(float fixedDelta) -> void = 0;
    };
}

#endif //SMB_IPHYSICSBACKEND_HPP
