//
// Created by drhaz on 03.07.2026.
//

#ifndef SMB_IIDENTITYCOMPONENT_HPP
#define SMB_IIDENTITYCOMPONENT_HPP

#include <string>

namespace engine::components {
    struct IIdentityComponent {
        std::string id;
        std::string name;
    };
}

#endif //SMB_IIDENTITYCOMPONENT_HPP
