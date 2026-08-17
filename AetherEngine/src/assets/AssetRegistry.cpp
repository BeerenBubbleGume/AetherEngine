#include "assets/AssetRegistry.hpp"

#include <algorithm>
#include <limits>
#include <unordered_set>

#include <nlohmann/json.hpp>

#include "security/PathSecurity.hpp"

namespace {
    using Json = nlohmann::json;
    using namespace AetherEngine::assets;

    constexpr std::uint64_t MaxManifestBytes = 4ULL * 1024ULL * 1024ULL;
    constexpr std::size_t MaxAssetRecords = 4096;
    constexpr std::size_t MaxVariantsPerAsset = 32;
    constexpr std::size_t MaxDependenciesPerAsset = 64;

    [[nodiscard]] auto parseAssetType(std::string_view value) -> std::optional<AssetType> {
        if (value == "mesh") return AssetType::Mesh;
        if (value == "texture") return AssetType::Texture;
        if (value == "shader") return AssetType::Shader;
        if (value == "material") return AssetType::Material;
        if (value == "scene") return AssetType::Scene;
        if (value == "audio") return AssetType::Audio;
        if (value == "script") return AssetType::Script;
        if (value == "prefab") return AssetType::Prefab;
        return std::nullopt;
    }

    [[nodiscard]] auto parseOperatingSystem(std::string_view value)
        -> std::optional<OperatingSystem> {
        if (value == "any") return OperatingSystem::Any;
        if (value == "windows") return OperatingSystem::Windows;
        if (value == "macos") return OperatingSystem::MacOS;
        if (value == "linux") return OperatingSystem::Linux;
        return std::nullopt;
    }

    [[nodiscard]] auto parseArchitecture(std::string_view value)
        -> std::optional<Architecture> {
        if (value == "any") return Architecture::Any;
        if (value == "x64") return Architecture::X64;
        if (value == "arm64") return Architecture::Arm64;
        return std::nullopt;
    }

    [[nodiscard]] auto parseGraphicsBackend(std::string_view value)
        -> std::optional<GraphicsBackend> {
        if (value == "any") return GraphicsBackend::Any;
        if (value == "d3d11") return GraphicsBackend::Direct3D11;
        if (value == "d3d12") return GraphicsBackend::Direct3D12;
        if (value == "vulkan") return GraphicsBackend::Vulkan;
        if (value == "metal") return GraphicsBackend::Metal;
        if (value == "opengl") return GraphicsBackend::OpenGL;
        if (value == "opengles") return GraphicsBackend::OpenGLES;
        return std::nullopt;
    }

    [[nodiscard]] auto safeRelativePath(const Json& value)
        -> std::optional<std::filesystem::path> {
        if (!value.is_string()) {
            return std::nullopt;
        }
        const auto text = value.get<std::string>();
        if (text.empty() || text.size() > 4096 || text.contains(':')) {
            return std::nullopt;
        }
        std::filesystem::path path{text};
        if (path.is_absolute() || path.has_root_path()) {
            return std::nullopt;
        }
        path = path.lexically_normal();
        if (path.empty() || path == "." ||
            (path.begin() != path.end() && *path.begin() == "..")) {
            return std::nullopt;
        }
        return path;
    }

    [[nodiscard]] auto variantScore(
        const PlatformProfile& candidate,
        const PlatformProfile& requested
    ) -> std::optional<int> {
        if (candidate.os != OperatingSystem::Any && candidate.os != requested.os) return std::nullopt;
        if (candidate.architecture != Architecture::Any && candidate.architecture != requested.architecture) {
            return std::nullopt;
        }
        if (candidate.graphics != GraphicsBackend::Any && candidate.graphics != requested.graphics) {
            return std::nullopt;
        }

        int score = 0;
        if (candidate.os == requested.os) score += 4;
        if (candidate.architecture == requested.architecture) score += 2;
        if (candidate.graphics == requested.graphics) score += 8;
        return score;
    }
}

namespace AetherEngine::assets {
    AssetRegistry::AssetRegistry(std::filesystem::path assetsRoot)
        : m_assetsRoot(std::move(assetsRoot)) {
        if (const auto canonical = security::canonicalDirectory(m_assetsRoot)) {
            m_assetsRoot = *canonical;
        } else {
            m_assetsRoot = m_assetsRoot.lexically_normal();
        }
    }

    auto AssetRegistry::loadManifest(const std::filesystem::path& manifest)
        -> std::expected<void, AssetError> {
        const auto source = security::readRegularFileWithin(
            m_assetsRoot,
            manifest.generic_string(),
            MaxManifestBytes
        );
        if (!source) {
            return std::unexpected(AssetError{1, "Failed to read asset manifest: " + source.error().message});
        }

        try {
            const auto body = Json::parse(source->bytes.begin(), source->bytes.end());
            if (!body.is_object() || body.value("version", 0) != 1) {
                return std::unexpected(AssetError{2, "Unsupported or malformed asset manifest"});
            }
            const auto assetsIt = body.find("assets");
            if (assetsIt == body.end() || !assetsIt->is_array() || assetsIt->size() > MaxAssetRecords) {
                return std::unexpected(AssetError{2, "Asset manifest must contain a bounded assets array"});
            }

            std::unordered_map<AssetId, AssetRecord, AssetIdHash> records;
            std::unordered_map<std::string, AssetId> aliases;
            records.reserve(assetsIt->size());
            aliases.reserve(assetsIt->size());

            for (const auto& assetBody : *assetsIt) {
                if (!assetBody.is_object()) {
                    return std::unexpected(AssetError{2, "Asset record must be an object"});
                }
                const auto id = parseAssetId(assetBody.value("id", std::string{}));
                const auto type = parseAssetType(assetBody.value("type", std::string{}));
                const auto alias = assetBody.value("alias", std::string{});
                if (!id || !type || alias.empty() || alias.size() > 256 || !alias.starts_with("asset://")) {
                    return std::unexpected(AssetError{2, "Asset record has an invalid id, type, or alias"});
                }

                AssetRecord record{.id = *id, .type = *type, .alias = alias};
                if (const auto dependencies = assetBody.find("dependencies"); dependencies != assetBody.end()) {
                    if (!dependencies->is_array() || dependencies->size() > MaxDependenciesPerAsset) {
                        return std::unexpected(AssetError{2, "Asset dependency list is invalid"});
                    }
                    std::unordered_set<AssetId, AssetIdHash> uniqueDependencies;
                    for (const auto& dependencyBody : *dependencies) {
                        if (!dependencyBody.is_string()) {
                            return std::unexpected(AssetError{2, "Asset dependency id must be a string"});
                        }
                        const auto dependency = parseAssetId(dependencyBody.get<std::string>());
                        if (!dependency || *dependency == *id || !uniqueDependencies.insert(*dependency).second) {
                            return std::unexpected(AssetError{2, "Asset dependency id is invalid or duplicated"});
                        }
                        record.dependencies.push_back(*dependency);
                    }
                }

                const auto variants = assetBody.find("variants");
                if (variants == assetBody.end() || !variants->is_array() || variants->empty() ||
                    variants->size() > MaxVariantsPerAsset) {
                    return std::unexpected(AssetError{2, "Asset must contain at least one bounded variant"});
                }
                for (const auto& variantBody : *variants) {
                    if (!variantBody.is_object()) {
                        return std::unexpected(AssetError{2, "Asset variant must be an object"});
                    }
                    const auto profileBody = variantBody.value("platform", Json::object());
                    if (!profileBody.is_object()) {
                        return std::unexpected(AssetError{2, "Asset platform profile must be an object"});
                    }
                    const auto os = parseOperatingSystem(profileBody.value("os", std::string{"any"}));
                    const auto architecture = parseArchitecture(
                        profileBody.value("architecture", std::string{"any"})
                    );
                    const auto graphics = parseGraphicsBackend(
                        profileBody.value("graphics", std::string{"any"})
                    );
                    if (!os || !architecture || !graphics) {
                        return std::unexpected(AssetError{2, "Asset platform profile contains an unknown value"});
                    }

                    AssetVariant variant{
                        .platform = {*os, *architecture, *graphics},
                        .contentHash = variantBody.value("contentHash", std::uint64_t{})
                    };
                    if (const auto pathIt = variantBody.find("path"); pathIt != variantBody.end()) {
                        const auto path = safeRelativePath(*pathIt);
                        if (!path) {
                            return std::unexpected(AssetError{2, "Asset variant path is unsafe"});
                        }
                        variant.cookedPath = *path;
                    }
                    if (const auto artifacts = variantBody.find("artifacts"); artifacts != variantBody.end()) {
                        if (!artifacts->is_object() || artifacts->size() > 16) {
                            return std::unexpected(AssetError{2, "Asset artifact map is invalid"});
                        }
                        for (const auto& [name, pathBody] : artifacts->items()) {
                            const auto path = safeRelativePath(pathBody);
                            if (name.empty() || name.size() > 64 || !path) {
                                return std::unexpected(AssetError{2, "Asset artifact entry is invalid"});
                            }
                            variant.artifacts.emplace(name, *path);
                        }
                    }
                    if (variant.cookedPath.empty() && variant.artifacts.empty()) {
                        return std::unexpected(AssetError{2, "Asset variant has no cooked data"});
                    }
                    record.variants.push_back(std::move(variant));
                }

                if (!records.emplace(record.id, std::move(record)).second ||
                    !aliases.emplace(alias, *id).second) {
                    return std::unexpected(AssetError{2, "Asset manifest contains duplicate ids or aliases"});
                }
            }

            for (const auto& [id, record] : records) {
                static_cast<void>(id);
                for (const auto dependency : record.dependencies) {
                    if (!records.contains(dependency)) {
                        return std::unexpected(AssetError{2, "Asset dependency does not exist in the manifest"});
                    }
                }
            }

            m_records = std::move(records);
            m_aliases = std::move(aliases);
            return {};
        } catch (const Json::exception& error) {
            return std::unexpected(AssetError{2, "Failed to parse asset manifest: " + std::string{error.what()}});
        } catch (const std::bad_alloc&) {
            return std::unexpected(AssetError{3, "Not enough memory to load asset manifest"});
        }
    }

    auto AssetRegistry::find(AssetId id) const -> const AssetRecord* {
        const auto found = m_records.find(id);
        return found == m_records.end() ? nullptr : &found->second;
    }

    auto AssetRegistry::findByAlias(std::string_view alias) const -> const AssetRecord* {
        const auto found = m_aliases.find(std::string{alias});
        return found == m_aliases.end() ? nullptr : find(found->second);
    }

    auto AssetRegistry::resolveVariant(AssetId id, const PlatformProfile& platform) const
        -> std::expected<AssetVariant, AssetError> {
        const auto* record = find(id);
        if (!record) {
            return std::unexpected(AssetError{1, "Unknown asset id: " + toString(id)});
        }

        const AssetVariant* best = nullptr;
        int bestScore = std::numeric_limits<int>::min();
        for (const auto& variant : record->variants) {
            const auto score = variantScore(variant.platform, platform);
            if (score && *score > bestScore) {
                best = &variant;
                bestScore = *score;
            }
        }
        if (!best) {
            return std::unexpected(AssetError{4, "No compatible variant for asset: " + record->alias});
        }
        return *best;
    }

    auto AssetRegistry::assetsRoot() const noexcept -> const std::filesystem::path& {
        return m_assetsRoot;
    }
}
