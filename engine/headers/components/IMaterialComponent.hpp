//
// Created by drhaz on 26.06.2026.
//

#ifndef SMB_IMATERIALCOMPONENT_HPP
#define SMB_IMATERIALCOMPONENT_HPP

#include <string>
#include <vector>

#include "resources/RTypes.hpp"

namespace engine::components {
    struct IMaterialComponent {
        resources::RMaterialHandle material;
        std::string programName;
        std::string vertexShaderPath;
        std::string fragmentShaderPath;
        std::vector<std::string> texturePaths;
    };
}

#endif //SMB_IMATERIALCOMPONENT_HPP
