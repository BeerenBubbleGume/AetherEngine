//
// Created by drhaz on 23.07.2026.
//

#ifndef SMB_PHYSICSTYPES_HPP
#define SMB_PHYSICSTYPES_HPP
#include <cstdint>
#include <string>

#include "components/PhysicsComponents.hpp"
#include "math/Types.hpp"

namespace engine::physics {
    struct BodyHandle {
        std::uint32_t value{};

        [[nodiscard]] constexpr auto isValid() const noexcept -> bool {
            return value != 0;
        }
    };

    struct BodyDesc {
        math::Transform transform;
        components::ColliderComponent collider;
        float mass{1.0f};
        bool dynamic{true};
        bool useGravity{true};
    };

    struct PhysicsError {
        int code{};
        std::string message;
    };
}

#endif //SMB_PHYSICSTYPES_HPP
