#ifndef AETHERENGINE_ASSETS_ASSETREGISTRY_HPP
#define AETHERENGINE_ASSETS_ASSETREGISTRY_HPP

#include <expected>
#include <filesystem>
#include <span>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "AssetTypes.hpp"

namespace AetherEngine::assets {
    class AssetRegistry final {
    public:
        explicit AssetRegistry(std::filesystem::path assetsRoot);

        auto loadManifest(const std::filesystem::path& manifest)
            -> std::expected<void, AssetError>;
        auto addAssets(std::span<const AssetRecord> records) -> std::expected<void, AssetError>;
        auto saveManifest(const std::filesystem::path& manifest) const -> std::expected<void, AssetError>;

        [[nodiscard]] auto find(AssetId id) const -> const AssetRecord*;
        [[nodiscard]] auto findByAlias(std::string_view alias) const -> const AssetRecord*;
        [[nodiscard]] auto resolveVariant(AssetId id, const PlatformProfile& platform) const
            -> std::expected<AssetVariant, AssetError>;
        [[nodiscard]] auto assetsRoot() const noexcept -> const std::filesystem::path&;
        [[nodiscard]] auto listAssets() const -> std::vector<AssetRecord>;
    private:
        auto loadContents(std::string_view contents) -> std::expected<void, AssetError>;
        std::filesystem::path m_assetsRoot;
        std::unordered_map<AssetId, AssetRecord, AssetIdHash> m_records;
        std::unordered_map<std::string, AssetId> m_aliases;
    };
}

#endif // AETHERENGINE_ASSETS_ASSETREGISTRY_HPP
