#ifndef AETHERENGINE_ASSETS_ASSETLOADERS_HPP
#define AETHERENGINE_ASSETS_ASSETLOADERS_HPP

#include <memory>

#include "IAssetLoader.hpp"

namespace AetherEngine::assets {
    [[nodiscard]] auto createMeshAssetLoader() -> std::unique_ptr<IAssetLoader>;
    [[nodiscard]] auto createTextureAssetLoader() -> std::unique_ptr<IAssetLoader>;
    [[nodiscard]] auto createShaderAssetLoader() -> std::unique_ptr<IAssetLoader>;
    [[nodiscard]] auto createMaterialAssetLoader() -> std::unique_ptr<IAssetLoader>;
}

#endif // AETHERENGINE_ASSETS_ASSETLOADERS_HPP
