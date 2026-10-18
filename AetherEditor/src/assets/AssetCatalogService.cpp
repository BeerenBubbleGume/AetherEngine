#include "assets/AssetCatalogService.hpp"

#include <algorithm>
#include <cctype>
#include <random>
#include <stdexcept>
#include <utility>
#include <unordered_set>
#include <nlohmann/json.hpp>
#include "assets/AssetManager.hpp"
#include "security/AssetValidation.hpp"
#include "security/PathSecurity.hpp"

namespace {
    namespace fs = std::filesystem;
    namespace engine = AetherEngine::assets;
    namespace security = AetherEngine::security;
    using Json = nlohmann::json;
    constexpr std::uint64_t MaxTextureBytes = 64ULL * 1024ULL * 1024ULL;

    auto pathString(const fs::path& path) -> std::string {
        const auto value = path.generic_u8string();
        return {value.begin(), value.end()};
    }

    auto utf8Path(std::string_view value) -> fs::path {
        return fs::path{std::u8string{value.begin(), value.end()}};
    }

    auto pathKey(const fs::path& path) -> std::string {
        auto value = pathString(path.lexically_normal());
#ifdef _WIN32
        std::ranges::transform(value, value.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
#endif
        return value;
    }

    auto newId(const engine::AssetRegistry& registry,
               const std::unordered_set<engine::AssetId, engine::AssetIdHash>& reserved = {}) -> engine::AssetId {
        std::random_device random;
        const auto word = [&] { return (static_cast<std::uint64_t>(random()) << 32U) | random(); };
        for (unsigned attempt = 0; attempt < 16; ++attempt) {
            engine::AssetId id{word(), word()};
            if (id.isValid() && !registry.find(id) && !reserved.contains(id)) return id;
        }
        throw std::runtime_error("Could not allocate a unique asset ID");
    }

    auto validateTexture(const fs::path& root, const std::string& relative) -> std::expected<void, engine::AssetError> {
        const auto file = security::readRegularFileWithin(root, relative, MaxTextureBytes);
        if (!file) return std::unexpected(engine::AssetError{1, file.error().message});
        if (!security::validateKtxPayload(file->bytes)) {
            return std::unexpected(engine::AssetError{2, "Unsupported or malformed KTX (expected 2D RGBA8 KTX1)"});
        }
        return {};
    }
}

namespace AetherEditor::assets {
    auto AssetCatalogService::scanTextures(engine::AssetManager& manager)
        -> std::expected<TextureScanReport, engine::AssetError> {
        try {
            const auto& registry = manager.registry();
            const auto& root = registry.assetsRoot();
            const auto directory = security::resolveDirectoryWithin(root, "textures/bin");
            if (!directory) return std::unexpected(engine::AssetError{1, directory.error().message});

            std::unordered_set<std::string> paths;
            std::unordered_set<std::string> aliases;
            for (const auto& record : registry.listAssets()) {
                aliases.insert(record.alias);
                if (record.type == engine::AssetType::Texture) {
                    for (const auto& variant : record.variants) paths.insert(pathKey(variant.cookedPath));
                }
            }

            TextureScanReport report;
            std::vector<engine::AssetRecord> additions;
            std::unordered_set<engine::AssetId, engine::AssetIdHash> ids;
            std::size_t visited = 0;
            for (fs::recursive_directory_iterator it(*directory), end; it != end; ++it) {
                if (++visited > 16384) return std::unexpected(engine::AssetError{2, "Texture scan entry limit reached"});
                const auto relative = it->path().lexically_relative(root);
                if (it->is_directory()) {
                    if (it.depth() >= 16 || !security::resolveDirectoryWithin(root, pathString(relative))) {
                        it.disable_recursion_pending();
                    }
                    continue;
                }
                if (!it->is_regular_file()) continue;
                auto extension = pathString(it->path().extension());
                std::ranges::transform(extension, extension.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
                if (extension != ".ktx") continue;

                const auto path = pathString(relative);
                if (paths.contains(pathKey(relative))) {
                    ++report.existing;
                    continue;
                }
                if (auto valid = validateTexture(root, path); !valid) {
                    report.warnings.push_back(path + ": " + valid.error().message);
                    continue;
                }
                auto name = it->path().lexically_relative(*directory);
                name.replace_extension();
                const auto alias = "asset://textures/" + pathString(name);
                if (alias.size() > 256 || aliases.contains(alias)) {
                    report.warnings.push_back(path + ": asset alias is too long or already used");
                    continue;
                }
                const auto id = newId(registry, ids);
                ids.insert(id);
                aliases.insert(alias);
                additions.push_back(engine::AssetRecord{
                    .id = id, .type = engine::AssetType::Texture, .alias = alias,
                    .variants = {engine::AssetVariant{.cookedPath = relative}}
                });
            }

            if (auto added = manager.registerAssets(additions); !added) return std::unexpected(added.error());
            report.added = additions.size();
            return report;
        } catch (const std::exception& error) {
            return std::unexpected(engine::AssetError{1, "Texture scan failed: " + std::string{error.what()}});
        }
    }

    auto AssetCatalogService::createMaterial(engine::AssetManager& manager, const CreateMaterialRequest& request)
        -> std::expected<engine::AssetId, engine::AssetError> {
        try {
            const auto& registry = manager.registry();
            if (!security::isSafeSceneName(request.name)) {
                return std::unexpected(engine::AssetError{2, "Material name is empty or contains invalid filename characters"});
            }
            const auto alias = "asset://materials/" + request.name;
            if (registry.findByAlias(alias)) return std::unexpected(engine::AssetError{2, "Material name is already registered"});
            const auto* shader = registry.find(request.shader);
            const auto* texture = registry.find(request.texture);
            if (!shader || shader->type != engine::AssetType::Shader || !texture || texture->type != engine::AssetType::Texture) {
                return std::unexpected(engine::AssetError{2, "Choose a registered shader and texture"});
            }
            if (auto variant = registry.resolveVariant(shader->id, manager.platform()); !variant) {
                return std::unexpected(variant.error());
            }
            const auto textureVariant = registry.resolveVariant(texture->id, manager.platform());
            if (!textureVariant) return std::unexpected(textureVariant.error());
            if (auto valid = validateTexture(registry.assetsRoot(), pathString(textureVariant->cookedPath)); !valid) {
                return std::unexpected(valid.error());
            }

            const auto& root = registry.assetsRoot();
            fs::create_directory(root / "materials");
            if (auto directory = security::resolveDirectoryWithin(root, "materials"); !directory) {
                return std::unexpected(engine::AssetError{1, directory.error().message});
            }
            const auto relative = "materials/" + request.name + ".material.json";
            auto id = newId(registry);
            Json body{
                {"version", 1}, {"assetId", engine::toString(id)}, {"shader", engine::toString(shader->id)},
                {"textures", {engine::toString(texture->id)}},
                {"renderState", {{"writeRgb", true}, {"writeAlpha", true}, {"writeDepth", true},
                    {"depthTest", true}, {"cullBackFaces", true}, {"alphaBlend", false}, {"msaa", true}}}
            };
            if (fs::exists(root / utf8Path(relative))) {
                const auto existing = security::readRegularFileWithin(root, relative, 1024ULL * 1024ULL);
                if (!existing) return std::unexpected(engine::AssetError{1, existing.error().message});
                auto saved = Json::parse(existing->bytes);
                const auto savedId = engine::parseAssetId(saved.value("assetId", std::string{}));
                if (!savedId || registry.find(*savedId)) return std::unexpected(engine::AssetError{2, "Material file already exists"});
                body["assetId"] = engine::toString(*savedId);
                if (body != saved) return std::unexpected(engine::AssetError{2, "Material file already exists"});
                id = *savedId; // Retry registration after a previous catalog write failure.
            } else {
                const auto serialized = body.dump(4);
                if (auto saved = security::writeRegularFileWithin(root, relative,
                        {reinterpret_cast<const std::uint8_t*>(serialized.data()), serialized.size()}, 1024ULL * 1024ULL); !saved) {
                    return std::unexpected(engine::AssetError{1, saved.error().message});
                }
            }

            const engine::AssetRecord record{
                .id = id, .type = engine::AssetType::Material, .alias = alias,
                .dependencies = {shader->id, texture->id},
                .variants = {engine::AssetVariant{.cookedPath = utf8Path(relative)}}
            };
            if (auto added = manager.registerAssets(std::span{&record, 1}); !added) {
                return std::unexpected(engine::AssetError{1, "Material file saved, but catalog update failed; retry Create: " + added.error().message});
            }
            return id;
        } catch (const std::exception& error) {
            return std::unexpected(engine::AssetError{1, "Material creation failed: " + std::string{error.what()}});
        }
    }
}
