#ifndef AETHEREDITOR_ASSETCATALOGSERVICE_HPP
#define AETHEREDITOR_ASSETCATALOGSERVICE_HPP

#include <expected>
#include <string>
#include <string_view>
#include <vector>
#include "assets/AssetTypes.hpp"

namespace AetherEngine::assets { class AssetManager; }

namespace AetherEditor::assets {
    struct TextureScanReport {
        std::size_t added{};
        std::size_t existing{};
        std::vector<std::string> warnings;
    };

    struct CreateMaterialRequest {
        std::string name;
        AetherEngine::assets::AssetId shader;
        AetherEngine::assets::AssetId texture;
    };

    class AssetCatalogService {
    public:
        [[nodiscard]] static auto scanTextures(AetherEngine::assets::AssetManager& manager)
            -> std::expected<TextureScanReport, AetherEngine::assets::AssetError>;
        [[nodiscard]] static auto createMaterial(
            AetherEngine::assets::AssetManager& manager, const CreateMaterialRequest& request
        ) -> std::expected<AetherEngine::assets::AssetId, AetherEngine::assets::AssetError>;
    };
}
#endif
