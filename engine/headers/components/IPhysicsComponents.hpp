//
// Created by drhaz on 03.07.2026.
//

#ifndef SMB_IPHYSICSCOMPONENTS_HPP
#define SMB_IPHYSICSCOMPONENTS_HPP
#include "math/UTypes.hpp"

namespace engine::components {
    struct IRigidbodyComponent {
        bool dynamic{true};
        float mass{1.0f};
        bool useGravity{true};
    };

    enum class ColliderType {
        Box,
        Sphere,
        Capsule
    };

    struct IColliderComponent {
        ColliderType type{ColliderType::Box};
        math::TVec3 size{1.0f, 1.0f, 1.0f};
        float radius{0.5f};
        float height{1.0f};
        bool trigger{false};
    };
}

#endif //SMB_IPHYSICSCOMPONENTS_HPP
