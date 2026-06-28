//
// Created by drhaz on 26.06.2026.
//

#ifndef SMB_CAMERAMATRICIES_HPP
#define SMB_CAMERAMATRICIES_HPP

#include "UTypes.hpp"
#include "components/ICameraComponent.hpp"
#include "components/ITransformComponent.hpp"

namespace engine::math {
    class CameraMatrices {
    public:
        static TMat4 makeView(const components::ITransformComponent & transform);
        static TMat4 makeProjection(const engine::components::ICameraComponent& camera, int width, int height);
    };
}

#endif //SMB_CAMERAMATRICIES_HPP
