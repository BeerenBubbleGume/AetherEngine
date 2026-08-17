#include "assets/AssetLoaders.hpp"

#include <algorithm>
#include <unordered_set>

#include <nlohmann/json.hpp>

#include "assets/AssetManager.hpp"
#include "security/PathSecurity.hpp"

namespace AetherEngine::assets {
    namespace {
        using Json = nlohmann::json;
        constexpr std::uint64_t MaxMaterialBytes = 1024ULL * 1024ULL;
        constexpr std::size_t MaxMaterialTextures = 16;

        class DependencyRollback final {
        public:
            explicit DependencyRollback(AssetManager& manager) : m_manager(&manager) {}
            DependencyRollback(const DependencyRollback&) = delete;
            auto operator=(const DependencyRollback&) -> DependencyRollback& = delete;

            ~DependencyRollback() {
                if (!m_committed) {
                    for (auto dependency = m_loaded.rbegin(); dependency != m_loaded.rend(); ++dependency) {
                        m_manager->unload(*dependency);
                    }
                }
            }

            auto add(AssetId id) -> void { m_loaded.push_back(id); }
            auto commit() noexcept -> void { m_committed = true; }

        private:
            AssetManager* m_manager{};
            std::vector<AssetId> m_loaded;
            bool m_committed{false};
        };

        [[nodiscard]] auto parseMaterialId(const Json& body, std::string_view field)
            -> std::expected<AssetId, AssetError> {
            const auto found = body.find(field);
            if (found == body.end() || !found->is_string()) {
                return std::unexpected(AssetError{7, "Material field is missing: " + std::string{field}});
            }
            const auto id = parseAssetId(found->get<std::string>());
            if (!id) {
                return std::unexpected(AssetError{7, "Material contains an invalid asset id: " + std::string{field}});
            }
            return *id;
        }

        [[nodiscard]] auto parseRenderState(const Json& value) -> resources::RenderState {
            resources::RenderState result{};
            if (!value.is_object()) {
                return result;
            }
            result.writeRgb = value.value("writeRgb", result.writeRgb);
            result.writeAlpha = value.value("writeAlpha", result.writeAlpha);
            result.writeDepth = value.value("writeDepth", result.writeDepth);
            result.depthTest = value.value("depthTest", result.depthTest);
            result.cullBackFaces = value.value("cullBackFaces", result.cullBackFaces);
            result.alphaBlend = value.value("alphaBlend", result.alphaBlend);
            result.msaa = value.value("msaa", result.msaa);
            return result;
        }

        class MaterialAssetLoader final : public IAssetLoader {
        public:
            [[nodiscard]] auto assetType() const noexcept -> AssetType override {
                return AssetType::Material;
            }

            [[nodiscard]] auto load(const AssetLoadContext& context)
                -> std::expected<std::unique_ptr<RuntimeAsset>, AssetError> override {
                if (context.variant.cookedPath.empty()) {
                    return std::unexpected(AssetError{7, "Material asset has no cooked path"});
                }
                const auto source = security::readRegularFileWithin(
                    context.assetsRoot,
                    context.variant.cookedPath.generic_string(),
                    MaxMaterialBytes
                );
                if (!source) {
                    return std::unexpected(AssetError{7, "Failed to read material asset: " + source.error().message});
                }

                try {
                    const auto body = Json::parse(source->bytes.begin(), source->bytes.end());
                    if (!body.is_object() || body.value("version", 0) != 1) {
                        return std::unexpected(AssetError{7, "Unsupported or malformed material asset"});
                    }
                    const auto shaderId = parseMaterialId(body, "shader");
                    if (!shaderId) {
                        return std::unexpected(shaderId.error());
                    }
                    const auto texturesBody = body.value("textures", Json::array());
                    if (!texturesBody.is_array() || texturesBody.size() > MaxMaterialTextures) {
                        return std::unexpected(AssetError{7, "Material texture list is invalid"});
                    }

                    std::vector<AssetId> textureIds;
                    textureIds.reserve(texturesBody.size());
                    std::unordered_set<AssetId, AssetIdHash> usedDependencies;
                    usedDependencies.insert(*shaderId);
                    for (const auto& textureBody : texturesBody) {
                        if (!textureBody.is_string()) {
                            return std::unexpected(AssetError{7, "Material texture id must be a string"});
                        }
                        const auto textureId = parseAssetId(textureBody.get<std::string>());
                        if (!textureId || !usedDependencies.insert(*textureId).second) {
                            return std::unexpected(AssetError{7, "Material texture dependency is invalid"});
                        }
                        textureIds.push_back(*textureId);
                    }

                    if (usedDependencies.size() != context.record.dependencies.size() ||
                        std::ranges::any_of(context.record.dependencies, [&](AssetId dependency) {
                            return !usedDependencies.contains(dependency);
                        })) {
                        return std::unexpected(AssetError{
                            7,
                            "Material dependencies do not exactly match the registry record"
                        });
                    }

                    const auto renderState = parseRenderState(
                        body.value("renderState", Json::object())
                    );
                    DependencyRollback rollback{context.manager};
                    auto shader = context.manager.load(
                        AssetRef<RuntimeShaderAsset>{*shaderId}
                    );
                    if (!shader) {
                        return std::unexpected(shader.error());
                    }
                    rollback.add(*shaderId);

                    std::vector<AssetHandle<RuntimeTextureAsset>> textures;
                    textures.reserve(textureIds.size());
                    for (const auto textureId : textureIds) {
                        auto texture = context.manager.load(
                            AssetRef<RuntimeTextureAsset>{textureId}
                        );
                        if (!texture) {
                            return std::unexpected(texture.error());
                        }
                        textures.push_back(*texture);
                        rollback.add(textureId);
                    }

                    std::unique_ptr<RuntimeAsset> result = std::make_unique<RuntimeMaterialAsset>(
                        *shader,
                        std::move(textures),
                        renderState
                    );
                    rollback.commit();
                    return result;
                } catch (const Json::exception& error) {
                    return std::unexpected(AssetError{7, "Failed to parse material asset: " + std::string{error.what()}});
                } catch (const std::bad_alloc&) {
                    return std::unexpected(AssetError{3, "Not enough memory to load material asset"});
                }
            }
        };
    }

    auto createMaterialAssetLoader() -> std::unique_ptr<IAssetLoader> {
        return std::make_unique<MaterialAssetLoader>();
    }
}
