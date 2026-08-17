//
// Created by drhaz on 24.06.2026.
//

#ifndef SMB_MESHCOMPONENT_HPP
#define SMB_MESHCOMPONENT_HPP

#include "assets/RuntimeAsset.hpp"

namespace AetherEngine::components {
    struct MeshComponent {
        assets::AssetRef<assets::RuntimeMeshAsset> asset;
        assets::AssetHandle<assets::RuntimeMeshAsset> runtime;
    };
}


#endif //SMB_MESHCOMPONENT_HPP
