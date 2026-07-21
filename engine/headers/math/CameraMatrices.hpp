//
// Created by drhaz on 26.06.2026.
//

#ifndef SMB_CAMERAMATRICES_HPP
#define SMB_CAMERAMATRICES_HPP

#include "Types.hpp"
#include "components/CameraComponent.hpp"
#include "components/TransformComponent.hpp"

namespace engine::math {
    class CameraMatrices {
    public:
        static Mat4 makeView(const components::TransformComponent & transform);
        static Mat4 makeProjection(const engine::components::CameraComponent& camera, int width, int height);
    };
}

#endif //SMB_CAMERAMATRICES_HPP
