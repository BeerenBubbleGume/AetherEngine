//
// Created by drhaz on 03.07.2026.
//

#ifndef SMB_PHYSICSCOMPONENTS_HPP
#define SMB_PHYSICSCOMPONENTS_HPP
#include "math/Types.hpp"

namespace AetherEngine::components {
    struct RigidbodyComponent {
        bool dynamic{true};
        float mass{1.0f};
        bool useGravity{true};
        auto operator==(const RigidbodyComponent&) const -> bool = default;
    };

    enum class ColliderType {
        Box,
        Sphere,
        Capsule
    };

    struct ColliderComponent {
        ColliderType type{ColliderType::Box};
        math::Vec3 size{1.0f, 1.0f, 1.0f};
        float radius{0.5f};
        float height{1.0f}; // Capsule cylinder length along local X in both backends.
        bool trigger{false};
        auto operator==(const ColliderComponent&) const -> bool = default;
    };
}

#endif //SMB_PHYSICSCOMPONENTS_HPP
