#include "assets/AssetManager.hpp"

#include <limits>
#include <new>

#include "assets/AssetLoaders.hpp"

namespace AetherEngine::assets {
    AssetManager::AssetManager(
        std::filesystem::path assetsRoot,
        resources::ResourceManager& resources,
        PlatformProfile platform
    ) : m_assetsRoot(std::move(assetsRoot)),
        m_resources(&resources),
        m_platform(platform),
        m_registry(m_assetsRoot) {
        static_cast<void>(registerLoader(createMeshAssetLoader()));
        static_cast<void>(registerLoader(createTextureAssetLoader()));
        static_cast<void>(registerLoader(createShaderAssetLoader()));
        static_cast<void>(registerLoader(createMaterialAssetLoader()));
    }

    auto AssetManager::create(
        const std::filesystem::path& assetsRoot,
        resources::ResourceManager& resources,
        PlatformProfile platform,
        const std::filesystem::path& manifest
    ) -> std::expected<std::unique_ptr<AssetManager>, AssetError> {
        auto manager = std::unique_ptr<AssetManager>(
            new AssetManager(assetsRoot, resources, platform)
        );
        if (auto loaded = manager->m_registry.loadManifest(manifest); !loaded) {
            return std::unexpected(loaded.error());
        }
        return manager;
    }

    auto AssetManager::registerLoader(std::unique_ptr<IAssetLoader> loader)
        -> std::expected<void, AssetError> {
        if (!loader) {
            return std::unexpected(AssetError{7, "Cannot register a null asset loader"});
        }
        const auto type = loader->assetType();
        if (m_loaders.contains(type)) {
            return std::unexpected(AssetError{7, "An asset loader for this type is already registered"});
        }
        m_loaders.emplace(type, std::move(loader));
        return {};
    }

    auto AssetManager::loadUntyped(AssetId id, AssetType expectedType)
        -> std::expected<UntypedHandle, AssetError> {
        if (!id.isValid()) {
            return std::unexpected(AssetError{1, "Cannot load an invalid asset id"});
        }
        const auto* record = m_registry.find(id);
        if (!record) {
            return std::unexpected(AssetError{1, "Unknown asset id: " + toString(id)});
        }
        if (record->type != expectedType) {
            return std::unexpected(AssetError{2, "Asset type mismatch for: " + record->alias});
        }

        if (const auto loaded = m_loaded.find(id); loaded != m_loaded.end()) {
            auto& entry = m_entries[loaded->second];
            if (entry.type != expectedType) {
                return std::unexpected(AssetError{2, "Loaded asset type mismatch for: " + record->alias});
            }
            if (entry.state == AssetLoadState::Loading) {
                return std::unexpected(AssetError{5, "Cyclic asset dependency detected at: " + record->alias});
            }
            if (entry.state == AssetLoadState::Failed) {
                return std::unexpected(entry.error.value_or(AssetError{6, "Asset previously failed to load"}));
            }
            if (entry.state == AssetLoadState::Unloaded) {
                entry.referenceCount = 1;
                if (auto result = loadEntry(loaded->second); !result) {
                    entry.referenceCount = 0;
                    return std::unexpected(result.error());
                }
            } else {
                if (entry.referenceCount == std::numeric_limits<std::uint32_t>::max()) {
                    return std::unexpected(AssetError{6, "Asset reference count overflow"});
                }
                ++entry.referenceCount;
            }
            return UntypedHandle{static_cast<std::uint32_t>(loaded->second), entry.generation};
        }

        if (m_entries.size() >= std::numeric_limits<std::uint32_t>::max()) {
            return std::unexpected(AssetError{6, "Asset entry limit reached"});
        }
        const auto index = m_entries.size();
        m_entries.push_back(LoadedAssetEntry{
            .id = id,
            .type = expectedType,
            .state = AssetLoadState::Unloaded,
            .generation = 1,
            .referenceCount = 1
        });
        m_loaded.emplace(id, index);
        if (auto result = loadEntry(index); !result) {
            m_entries[index].referenceCount = 0;
            return std::unexpected(result.error());
        }
        return UntypedHandle{static_cast<std::uint32_t>(index), m_entries[index].generation};
    }

    auto AssetManager::loadEntry(std::size_t index) -> std::expected<void, AssetError> {
        if (index >= m_entries.size() || !m_resources) {
            return std::unexpected(AssetError{6, "Asset manager is not initialized"});
        }
        auto& entry = m_entries[index];
        const auto* record = m_registry.find(entry.id);
        if (!record) {
            entry.state = AssetLoadState::Failed;
            entry.error = AssetError{1, "Asset disappeared from the registry"};
            return std::unexpected(*entry.error);
        }
        const auto loader = m_loaders.find(record->type);
        if (loader == m_loaders.end() || !loader->second) {
            entry.state = AssetLoadState::Failed;
            entry.error = AssetError{7, "No runtime loader registered for: " + record->alias};
            return std::unexpected(*entry.error);
        }
        const auto variant = m_registry.resolveVariant(entry.id, m_platform);
        if (!variant) {
            entry.state = AssetLoadState::Failed;
            entry.error = variant.error();
            return std::unexpected(*entry.error);
        }

        entry.state = AssetLoadState::Loading;
        entry.object.reset();
        entry.error.reset();
        const AssetLoadContext context{
            .record = *record,
            .variant = *variant,
            .assetsRoot = m_assetsRoot,
            .resources = *m_resources,
            .manager = *this
        };
        auto object = [&]() -> std::expected<std::unique_ptr<RuntimeAsset>, AssetError> {
            try {
                return loader->second->load(context);
            } catch (const std::bad_alloc&) {
                return std::unexpected(AssetError{3, "Not enough memory to load runtime asset"});
            } catch (const std::exception& error) {
                return std::unexpected(AssetError{7, "Runtime asset loader failed: " + std::string{error.what()}});
            }
        }();
        if (!object || !*object || (*object)->type() != record->type) {
            entry.state = AssetLoadState::Failed;
            entry.error = object
                ? AssetError{7, "Asset loader returned an incompatible runtime object"}
                : object.error();
            return std::unexpected(*entry.error);
        }

        entry.object = std::move(*object);
        entry.dependenciesLoaded = !record->dependencies.empty();
        entry.state = AssetLoadState::Ready;
        return {};
    }

    auto AssetManager::getUntyped(UntypedHandle handle, AssetType expectedType) const
        -> const RuntimeAsset* {
        if (handle.generation == 0 || handle.index >= m_entries.size()) {
            return nullptr;
        }
        const auto& entry = m_entries[handle.index];
        if (entry.generation != handle.generation || entry.type != expectedType ||
            entry.state != AssetLoadState::Ready || !entry.object) {
            return nullptr;
        }
        return entry.object.get();
    }

    auto AssetManager::releaseDependencies(const AssetRecord& record) -> void {
        for (const auto dependency : record.dependencies) {
            unload(dependency);
        }
    }

    auto AssetManager::unload(AssetId id) -> void {
        const auto loaded = m_loaded.find(id);
        if (loaded == m_loaded.end()) {
            return;
        }
        auto& entry = m_entries[loaded->second];
        if (entry.referenceCount > 1) {
            --entry.referenceCount;
            return;
        }
        if (entry.referenceCount == 0) {
            return;
        }

        entry.referenceCount = 0;
        entry.object.reset();
        entry.state = AssetLoadState::Unloaded;
        entry.error.reset();
        ++entry.generation;
        if (entry.generation == 0) {
            entry.generation = 1;
        }
        if (const auto* record = m_registry.find(id); record && entry.dependenciesLoaded) {
            releaseDependencies(*record);
            entry.dependenciesLoaded = false;
        }
    }

    auto AssetManager::reload(AssetId id) -> std::expected<void, AssetError> {
        const auto loaded = m_loaded.find(id);
        if (loaded == m_loaded.end()) {
            return std::unexpected(AssetError{1, "Cannot reload an asset that has not been loaded"});
        }
        auto& entry = m_entries[loaded->second];
        if (entry.state == AssetLoadState::Loading) {
            return std::unexpected(AssetError{5, "Cannot reload an asset while it is loading"});
        }
        if (const auto* record = m_registry.find(id); record && entry.dependenciesLoaded) {
            entry.object.reset();
            releaseDependencies(*record);
            entry.dependenciesLoaded = false;
        } else {
            entry.object.reset();
        }
        if (entry.referenceCount == 0) {
            entry.referenceCount = 1;
        }
        return loadEntry(loaded->second);
    }

    auto AssetManager::registry() const noexcept -> const AssetRegistry& {
        return m_registry;
    }

    auto AssetManager::platform() const noexcept -> PlatformProfile {
        return m_platform;
    }
}
