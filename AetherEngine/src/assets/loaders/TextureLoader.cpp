#include "assets/AssetLoaders.hpp"

#include <new>

namespace AetherEngine::assets {
    namespace {
        class TextureAssetLoader final : public IAssetLoader {
        public:
            [[nodiscard]] auto assetType() const noexcept -> AssetType override {
                return AssetType::Texture;
            }

            [[nodiscard]] auto load(const AssetLoadContext& context)
                -> std::expected<std::unique_ptr<RuntimeAsset>, AssetError> override {
                if (context.variant.cookedPath.empty()) {
                    return std::unexpected(AssetError{7, "Texture asset has no cooked path"});
                }
                const auto handle = context.resources.loadTexture(
                    context.variant.cookedPath.generic_string()
                );
                if (!handle.isValid()) {
                    return std::unexpected(AssetError{7, "Failed to load texture asset: " + context.record.alias});
                }
                try {
                    std::unique_ptr<RuntimeAsset> result =
                        std::make_unique<RuntimeTextureAsset>(context.resources, handle);
                    return result;
                } catch (const std::bad_alloc&) {
                    context.resources.releaseTexture(handle);
                    return std::unexpected(AssetError{3, "Not enough memory to create texture runtime asset"});
                }
            }
        };
    }

    auto createTextureAssetLoader() -> std::unique_ptr<IAssetLoader> {
        return std::make_unique<TextureAssetLoader>();
    }
}
