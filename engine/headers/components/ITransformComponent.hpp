//
// Created by drhaz on 24.06.2026.
//

#ifndef SMB_ITRANSFORMCOMPONENT_HPP
#define SMB_ITRANSFORMCOMPONENT_HPP

#include "graphics/GMesh.hpp"

namespace engine::components {
    struct ITransformComponent {
        graphics::GTransform transform;
    };
}


#endif //SMB_ITRANSFORMCOMPONENT_HPP
