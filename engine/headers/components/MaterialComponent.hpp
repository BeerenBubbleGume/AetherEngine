//
// Created by drhaz on 26.06.2026.
//

#ifndef SMB_MATERIALCOMPONENT_HPP
#define SMB_MATERIALCOMPONENT_HPP

#include <string>
#include <vector>

#include "resources/ResourceTypes.hpp"

namespace engine::components {
    struct MaterialComponent {
        resources::Material material;
        std::string programName;
        std::string vertexShaderPath;
        std::string fragmentShaderPath;
        std::vector<std::string> texturePaths;
    };
}

#endif //SMB_MATERIALCOMPONENT_HPP
