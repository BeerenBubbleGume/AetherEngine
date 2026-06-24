//
// Created by drhaz on 24.06.2026.
//

#ifndef SMB_IMESHCOMPONENT_HPP
#define SMB_IMESHCOMPONENT_HPP

#include "resources/RResourceManager.hpp"

namespace engine::components {
    struct IMeshComponent {
        resources::RMeshHandle mesh;
        resources::RProgramHandle program;
    };
}


#endif //SMB_IMESHCOMPONENT_HPP
