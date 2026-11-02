#pragma once

#include <cmath>
#include <string_view>

#include "components/PhysicsComponents.hpp"

namespace AetherEngine::physics {
    inline constexpr float MaxBodyMass = 1'000'000'000.0f;
    inline constexpr float MaxColliderDimension = 100'000.0f;

    [[nodiscard]] inline auto validPositivePhysicsValue(float value, float maximum) -> bool {
        return std::isfinite(value) && value > 0.0f && value <= maximum;
    }

    [[nodiscard]] inline auto invalidPhysicsField(const components::RigidbodyComponent& value) -> std::string_view {
        return validPositivePhysicsValue(value.mass, MaxBodyMass) ? "" : "Rigidbody.mass";
    }

    [[nodiscard]] inline auto invalidPhysicsField(const components::ColliderComponent& value) -> std::string_view {
        using components::ColliderType;
        if (value.type != ColliderType::Box && value.type != ColliderType::Sphere && value.type != ColliderType::Capsule)
            return "Collider.type";
        if (!validPositivePhysicsValue(value.size.x, MaxColliderDimension) ||
            !validPositivePhysicsValue(value.size.y, MaxColliderDimension) ||
            !validPositivePhysicsValue(value.size.z, MaxColliderDimension))
            return "Collider.size";
        if (!validPositivePhysicsValue(value.radius, MaxColliderDimension)) return "Collider.radius";
        if (!validPositivePhysicsValue(value.height, MaxColliderDimension)) return "Collider.height";
        return "";
    }
}
