//
// Created by drhaz on 24.06.2026.
//

#ifndef SMB_MESHCOMPONENT_HPP
#define SMB_MESHCOMPONENT_HPP

#include <string>

#include "resources/ResourceTypes.hpp"

namespace engine::components {
    struct MeshComponent {
        resources::MeshHandle mesh;
        std::string assetPath;
    };
}


#endif //SMB_MESHCOMPONENT_HPP
