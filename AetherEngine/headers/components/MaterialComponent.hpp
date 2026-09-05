//
// Created by drhaz on 26.06.2026.
//

#ifndef SMB_MATERIALCOMPONENT_HPP
#define SMB_MATERIALCOMPONENT_HPP

#include "assets/RuntimeAsset.hpp"
#include "math/Types.hpp"

namespace AetherEngine::components {
    struct MaterialComponent {
        assets::AssetRef<assets::RuntimeMaterialAsset> asset;
        math::Color baseColor{1.0f, 1.0f, 1.0f, 1.0f};
    };
}

#endif //SMB_MATERIALCOMPONENT_HPP
