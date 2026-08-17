#include "assets/AssetLoaders.hpp"

#include <new>

namespace AetherEngine::assets {
    namespace {
        class ShaderAssetLoader final : public IAssetLoader {
        public:
            [[nodiscard]] auto assetType() const noexcept -> AssetType override {
                return AssetType::Shader;
            }

            [[nodiscard]] auto load(const AssetLoadContext& context)
                -> std::expected<std::unique_ptr<RuntimeAsset>, AssetError> override {
                const auto vertex = context.variant.artifacts.find("vertex");
                const auto fragment = context.variant.artifacts.find("fragment");
                if (vertex == context.variant.artifacts.end() ||
                    fragment == context.variant.artifacts.end()) {
                    return std::unexpected(AssetError{7, "Shader asset requires vertex and fragment artifacts"});
                }
                const auto handle = context.resources.loadProgram(
                    context.record.alias,
                    vertex->second.generic_string(),
                    fragment->second.generic_string()
                );
                if (!handle.isValid()) {
                    return std::unexpected(AssetError{7, "Failed to load shader asset: " + context.record.alias});
                }
                try {
                    std::unique_ptr<RuntimeAsset> result =
                        std::make_unique<RuntimeShaderAsset>(context.resources, handle);
                    return result;
                } catch (const std::bad_alloc&) {
                    context.resources.releaseProgram(handle);
                    return std::unexpected(AssetError{3, "Not enough memory to create shader runtime asset"});
                }
            }
        };
    }

    auto createShaderAssetLoader() -> std::unique_ptr<IAssetLoader> {
        return std::make_unique<ShaderAssetLoader>();
    }
}
