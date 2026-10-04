//
// Created by drhaz on 03.07.2026.
//

#ifndef SMB_IDENTITYCOMPONENT_HPP
#define SMB_IDENTITYCOMPONENT_HPP

#include <string>
#include "scene/EntityId.hpp"

namespace AetherEngine::components {
    struct IdentityComponent {
        scene::EntityId id;
        std::string name;
    };
}

#endif //SMB_IDENTITYCOMPONENT_HPP
