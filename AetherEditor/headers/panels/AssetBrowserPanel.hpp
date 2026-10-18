//
// Created by drhaz on 08.10.2026.
//

#ifndef SMB_ASSETBROWSERPANEL_HPP
#define SMB_ASSETBROWSERPANEL_HPP
#include <memory>
#include <vector>
#include <optional>

#include "assets/AssetTypes.hpp"
#include "assets/AssetCatalogService.hpp"


namespace AetherEditor::panels {
    struct AssetBrowserFrame {
        bool scanTexturesRequested = false;
        std::optional<assets::CreateMaterialRequest> createMaterial;
    };

    class AssetBrowserPanel {
    public:
        struct AssetBrowserPanelDeleter {
            void operator()(AssetBrowserPanel* panel) const {
                delete panel;
            }
        };
        using AssetBrowserPanelPtr = std::unique_ptr<AssetBrowserPanel, AssetBrowserPanelDeleter>;
        static auto createAssetBrowserPanel() -> AssetBrowserPanelPtr;
        AssetBrowserPanel(const AssetBrowserPanel&) = delete;
        AssetBrowserPanel& operator=(const AssetBrowserPanel&) = delete;
        AssetBrowserPanel(AssetBrowserPanel&&) = delete;
        AssetBrowserPanel& operator=(AssetBrowserPanel&&) = delete;

        auto draw(bool &open, const std::vector<AetherEngine::assets::AssetRecord>& assetList) -> AssetBrowserFrame;
        auto setStatus(std::string message, bool error) -> void;
        auto materialCreated(AetherEngine::assets::AssetId id) -> void;

    private:
        using AssetType = AetherEngine::assets::AssetType;

        struct AssetGroup {
            AssetType type;
            const char* label;
        };

        static constexpr AssetGroup groups[] = {
            {AssetType::Mesh,     "Meshes"},
            {AssetType::Texture,  "Textures"},
            {AssetType::Shader,   "Shaders"},
            {AssetType::Material, "Materials"},
            {AssetType::Scene,    "Scenes"},
            {AssetType::Audio,    "Audio"},
            {AssetType::Script,   "Scripts"},
            {AssetType::Prefab,   "Prefabs"}
        };
        auto drawAssetDetails(const AetherEngine::assets::AssetRecord &asset) -> void;
        auto drawCreateMaterial(const std::vector<AetherEngine::assets::AssetRecord>& assetList, AssetBrowserFrame& frame) -> void;
        AssetBrowserPanel() = default;
        ~AssetBrowserPanel() = default;
        std::optional<AetherEngine::assets::AssetId> m_selectedAsset;
        std::string m_materialName;
        std::optional<AetherEngine::assets::AssetId> m_materialShader;
        std::optional<AetherEngine::assets::AssetId> m_materialTexture;
        bool m_openMaterialPopup = false;
        bool m_closeMaterialPopup = false;
        std::string m_status;
        bool m_statusError = false;
    };
} // panels
// AetherEditor

#endif //SMB_ASSETBROWSERPANEL_HPP
