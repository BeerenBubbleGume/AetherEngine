#ifndef AETHERENGINE_ASSETS_ASSETID_HPP
#define AETHERENGINE_ASSETS_ASSETID_HPP

#include <charconv>
#include <compare>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace AetherEngine::assets {
    struct AssetId {
        std::uint64_t high{};
        std::uint64_t low{};

        [[nodiscard]] constexpr auto isValid() const noexcept -> bool {
            return high != 0 || low != 0;
        }

        auto operator<=>(const AssetId&) const = default;
    };

    struct AssetIdHash {
        [[nodiscard]] auto operator()(AssetId id) const noexcept -> std::size_t {
            const auto mixed = id.high ^ (id.low + 0x9e3779b97f4a7c15ULL +
                (id.high << 6U) + (id.high >> 2U));
            return static_cast<std::size_t>(mixed);
        }
    };

    [[nodiscard]] inline auto parseAssetId(std::string_view value) -> std::optional<AssetId> {
        if (value.size() != 32) {
            return std::nullopt;
        }

        AssetId result{};
        const auto highResult = std::from_chars(value.data(), value.data() + 16, result.high, 16);
        const auto lowResult = std::from_chars(
            value.data() + 16,
            value.data() + value.size(),
            result.low,
            16
        );
        if (highResult.ec != std::errc{} || highResult.ptr != value.data() + 16 ||
            lowResult.ec != std::errc{} || lowResult.ptr != value.data() + value.size() ||
            !result.isValid()) {
            return std::nullopt;
        }
        return result;
    }

    [[nodiscard]] inline auto toString(AssetId id) -> std::string {
        constexpr char HexDigits[] = "0123456789abcdef";
        std::string result(32, '0');
        for (std::size_t index = 0; index < 16; ++index) {
            const auto shift = static_cast<unsigned>((15 - index) * 4);
            result[index] = HexDigits[(id.high >> shift) & 0x0fU];
            result[index + 16] = HexDigits[(id.low >> shift) & 0x0fU];
        }
        return result;
    }

    template<typename T>
    struct AssetRef {
        AssetId id{};

        [[nodiscard]] constexpr auto isValid() const noexcept -> bool {
            return id.isValid();
        }
    };
}

#endif // AETHERENGINE_ASSETS_ASSETID_HPP
