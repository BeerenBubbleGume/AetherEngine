//
// Created by drhaz on 25.06.2026.
//

#ifndef SMB_DETAIL_HPP
#define SMB_DETAIL_HPP

#include "UTypes.hpp"

#include <glm/fwd.hpp>

namespace engine::math::detail {
    [[nodiscard]] glm::vec3 toGlm(const TVec3& v);
    [[nodiscard]] glm::quat toGlm(const TQuat& q);
    [[nodiscard]] glm::mat4 toGlm(const TMat4& m);

    [[nodiscard]] TVec3 fromGlm(const glm::vec3& v);
    [[nodiscard]] TQuat fromGlm(const glm::quat& q);
    [[nodiscard]] TMat4 fromGlm(const glm::mat4& m);

} // namespace engine::math::detail

#endif //SMB_DETAIL_HPP
