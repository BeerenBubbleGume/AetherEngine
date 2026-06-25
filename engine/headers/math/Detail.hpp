//
// Created by drhaz on 25.06.2026.
//

#ifndef SMB_DETAIL_HPP
#define SMB_DETAIL_HPP

#include "UTypes.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace engine::math::detail {
    [[nodiscard]] glm::vec3 toGlm(const TVec3& v);
    [[nodiscard]] glm::quat toGlm(const TQuat& q);

    [[nodiscard]] TVec3 fromGlm(const glm::vec3& v);
    [[nodiscard]] TQuat fromGlm(const glm::quat& q);

} // namespace engine::math::detail

#endif //SMB_DETAIL_HPP
