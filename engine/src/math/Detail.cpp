//
// Created by drhaz on 25.06.2026.
//

#include "math/Detail.hpp"

#include <algorithm>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace engine::math::detail {
    glm::vec3 toGlm(const TVec3& v) {
        return {v.x, v.y, v.z};
    }

    glm::quat toGlm(const TQuat& q) {
        return glm::quat::wxyz(q.w, q.x, q.y, q.z);
    }

    glm::mat4 toGlm(const TMat4& m) {
        return glm::make_mat4(m.data());
    }

    TVec3 fromGlm(const glm::vec3& v) {
        return {v.x, v.y, v.z};
    }

    TQuat fromGlm(const glm::quat& q) {
        return {q.w, q.x, q.y, q.z};
    }

    TMat4 fromGlm(const glm::mat4& m) {
        TMat4 result = TMat4::zero();
        const auto* values = glm::value_ptr(m);
        std::copy(values, values + result.m.size(), result.m.begin());
        return result;
    }
} // namespace engine::math::detail
