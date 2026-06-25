//
// Created by drhaz on 25.06.2026.
//

#include "math/Detail.hpp"

namespace engine::math::detail {
    glm::vec3 toGlm(const TVec3& v) {
        return {v.x, v.y, v.z};
    }

    glm::quat toGlm(const TQuat& q) {
        return glm::quat::wxyz(q.w, q.x, q.y, q.z);
    }

    TVec3 fromGlm(const glm::vec3& v) {
        return {v.x, v.y, v.z};
    }

    TQuat fromGlm(const glm::quat& q) {
        return {q.w, q.x, q.y, q.z};
    }
} // namespace engine::math::detail
