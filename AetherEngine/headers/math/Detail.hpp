//
// Created by drhaz on 25.06.2026.
//

#ifndef SMB_DETAIL_HPP
#define SMB_DETAIL_HPP

#include "Types.hpp"

#include <glm/fwd.hpp>

namespace AetherEngine::math::detail {
    [[nodiscard]] glm::vec3 toGlm(const Vec3& v);
    [[nodiscard]] glm::quat toGlm(const Quat& q);
    [[nodiscard]] glm::mat4 toGlm(const Mat4& m);

    [[nodiscard]] Vec3 fromGlm(const glm::vec3& v);
    [[nodiscard]] Quat fromGlm(const glm::quat& q);
    [[nodiscard]] Mat4 fromGlm(const glm::mat4& m);

} // namespace AetherEngine::math::detail

#endif //SMB_DETAIL_HPP
