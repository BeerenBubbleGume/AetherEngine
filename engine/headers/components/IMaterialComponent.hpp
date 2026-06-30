//
// Created by drhaz on 26.06.2026.
//

#ifndef SMB_IMATERIALCOMPONENT_HPP
#define SMB_IMATERIALCOMPONENT_HPP

#include "resources/RTypes.hpp"

namespace engine::components {
    struct IMaterialComponent {
        resources::RMaterialHandle program;
    };
}

#endif //SMB_IMATERIALCOMPONENT_HPP
