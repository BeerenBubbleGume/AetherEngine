#ifndef AETHERENGINE_ASSETS_ASSETMETADATA_HPP
#define AETHERENGINE_ASSETS_ASSETMETADATA_HPP

#include <filesystem>
#include <string>

#include "AssetTypes.hpp"

namespace AetherEngine::assets {
    struct AssetMetadata {
        AssetId id{};
        AssetType type{AssetType::Mesh};
        std::string alias;
        std::filesystem::path sourcePath;
        std::string importer;
    };
}

#endif // AETHERENGINE_ASSETS_ASSETMETADATA_HPP
