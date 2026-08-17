#ifndef AETHERENGINE_ASSETS_ASSETTYPES_HPP
#define AETHERENGINE_ASSETS_ASSETTYPES_HPP

#include <cstdint>
#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

#include "AssetId.hpp"
#include "PlatformProfile.hpp"

namespace AetherEngine::assets {
    struct AssetError {
        int code{};
        std::string message;
    };

    enum class AssetType {
        Mesh,
        Texture,
        Shader,
        Material,
        Scene,
        Audio,
        Script,
        Prefab
    };

    struct AssetTypeHash {
        [[nodiscard]] auto operator()(AssetType value) const noexcept -> std::size_t {
            return static_cast<std::size_t>(value);
        }
    };

    struct AssetVariant {
        PlatformProfile platform{};
        std::filesystem::path cookedPath;
        std::unordered_map<std::string, std::filesystem::path> artifacts;
        std::uint64_t contentHash{};
    };

    struct AssetRecord {
        AssetId id{};
        AssetType type{AssetType::Mesh};
        std::string alias;
        std::vector<AssetId> dependencies;
        std::vector<AssetVariant> variants;
    };

    template<typename T>
    struct AssetHandle {
        std::uint32_t index{};
        std::uint32_t generation{};

        [[nodiscard]] constexpr auto isValid() const noexcept -> bool {
            return generation != 0;
        }

        bool operator==(const AssetHandle&) const = default;
    };
}

#endif // AETHERENGINE_ASSETS_ASSETTYPES_HPP
