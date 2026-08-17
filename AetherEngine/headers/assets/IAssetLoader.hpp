#ifndef AETHERENGINE_ASSETS_IASSETLOADER_HPP
#define AETHERENGINE_ASSETS_IASSETLOADER_HPP

#include <expected>
#include <filesystem>
#include <memory>

#include "AssetTypes.hpp"
#include "RuntimeAsset.hpp"

namespace AetherEngine::assets {
    class AssetManager;

    struct AssetLoadContext {
        const AssetRecord& record;
        const AssetVariant& variant;
        const std::filesystem::path& assetsRoot;
        resources::ResourceManager& resources;
        AssetManager& manager;
    };

    class IAssetLoader {
    public:
        virtual ~IAssetLoader() = default;

        [[nodiscard]] virtual auto assetType() const noexcept -> AssetType = 0;
        [[nodiscard]] virtual auto load(const AssetLoadContext& context)
            -> std::expected<std::unique_ptr<RuntimeAsset>, AssetError> = 0;
    };
}

#endif // AETHERENGINE_ASSETS_IASSETLOADER_HPP
