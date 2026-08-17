#ifndef AETHERENGINE_ASSETS_ASSETMANAGER_HPP
#define AETHERENGINE_ASSETS_ASSETMANAGER_HPP

#include <deque>
#include <expected>
#include <filesystem>
#include <memory>
#include <optional>
#include <type_traits>
#include <unordered_map>

#include "AssetRegistry.hpp"
#include "IAssetLoader.hpp"
#include "RuntimeAsset.hpp"

namespace AetherEngine::assets {
    enum class AssetLoadState {
        Unloaded,
        Loading,
        Ready,
        Failed
    };

    struct LoadedAssetEntry {
        AssetId id{};
        AssetType type{AssetType::Mesh};
        AssetLoadState state{AssetLoadState::Unloaded};
        std::uint32_t generation{1};
        std::uint32_t referenceCount{};
        bool dependenciesLoaded{false};
        std::unique_ptr<RuntimeAsset> object;
        std::optional<AssetError> error;
    };

    class AssetManager final {
    public:
        AssetManager(const AssetManager&) = delete;
        auto operator=(const AssetManager&) -> AssetManager& = delete;

        [[nodiscard]] static auto create(
            const std::filesystem::path& assetsRoot,
            resources::ResourceManager& resources,
            PlatformProfile platform,
            const std::filesystem::path& manifest = "registry/runtime-assets.json"
        ) -> std::expected<std::unique_ptr<AssetManager>, AssetError>;

        auto registerLoader(std::unique_ptr<IAssetLoader> loader)
            -> std::expected<void, AssetError>;

        template<typename T>
        [[nodiscard]] auto reference(std::string_view alias) const
            -> std::expected<AssetRef<T>, AssetError>;

        template<typename T>
        [[nodiscard]] auto load(AssetRef<T> reference)
            -> std::expected<AssetHandle<T>, AssetError>;

        template<typename T>
        [[nodiscard]] auto get(AssetHandle<T> handle) const -> const T*;

        auto unload(AssetId id) -> void;
        [[nodiscard]] auto reload(AssetId id) -> std::expected<void, AssetError>;

        [[nodiscard]] auto registry() const noexcept -> const AssetRegistry&;
        [[nodiscard]] auto platform() const noexcept -> PlatformProfile;

    private:
        struct UntypedHandle {
            std::uint32_t index{};
            std::uint32_t generation{};
        };

        AssetManager(
            std::filesystem::path assetsRoot,
            resources::ResourceManager& resources,
            PlatformProfile platform
        );

        [[nodiscard]] auto loadUntyped(AssetId id, AssetType expectedType)
            -> std::expected<UntypedHandle, AssetError>;
        [[nodiscard]] auto loadEntry(std::size_t index) -> std::expected<void, AssetError>;
        [[nodiscard]] auto getUntyped(UntypedHandle handle, AssetType expectedType) const
            -> const RuntimeAsset*;
        auto releaseDependencies(const AssetRecord& record) -> void;

        std::filesystem::path m_assetsRoot;
        resources::ResourceManager* m_resources{};
        PlatformProfile m_platform{};
        AssetRegistry m_registry;
        // Loading a material can recursively load its shader and textures. A deque
        // keeps references to the currently-loading entry stable while it grows.
        std::deque<LoadedAssetEntry> m_entries;
        std::unordered_map<AssetId, std::size_t, AssetIdHash> m_loaded;
        std::unordered_map<AssetType, std::unique_ptr<IAssetLoader>, AssetTypeHash> m_loaders;
    };

    template<typename T>
    auto AssetManager::reference(std::string_view alias) const
        -> std::expected<AssetRef<T>, AssetError> {
        static_assert(std::is_base_of_v<RuntimeAsset, T>);
        const auto* record = m_registry.findByAlias(alias);
        if (!record) {
            return std::unexpected(AssetError{1, "Unknown asset alias: " + std::string{alias}});
        }
        if (record->type != AssetTraits<T>::type) {
            return std::unexpected(AssetError{2, "Asset alias has an incompatible type: " + std::string{alias}});
        }
        return AssetRef<T>{record->id};
    }

    template<typename T>
    auto AssetManager::load(AssetRef<T> reference)
        -> std::expected<AssetHandle<T>, AssetError> {
        static_assert(std::is_base_of_v<RuntimeAsset, T>);
        const auto loaded = loadUntyped(reference.id, AssetTraits<T>::type);
        if (!loaded) {
            return std::unexpected(loaded.error());
        }
        return AssetHandle<T>{loaded->index, loaded->generation};
    }

    template<typename T>
    auto AssetManager::get(AssetHandle<T> handle) const -> const T* {
        static_assert(std::is_base_of_v<RuntimeAsset, T>);
        const auto* object = getUntyped(
            UntypedHandle{handle.index, handle.generation},
            AssetTraits<T>::type
        );
        return dynamic_cast<const T*>(object);
    }
}

#endif // AETHERENGINE_ASSETS_ASSETMANAGER_HPP
