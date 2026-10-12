//
// Created by drhaz on 28.09.2026.
//

#ifndef SMB_ENTITYID_HPP
#define SMB_ENTITYID_HPP

#include <charconv>
#include <chrono>
#include <memory>
#include <string_view>
#include <optional>
#include <cinttypes>
#include <random>

namespace AetherEngine::scene {
    struct EntityId {
        std::uint64_t high{};
        std::uint64_t low{};

        [[nodiscard]] constexpr auto isValid() const noexcept -> bool {
            return high != 0 || low != 0;
        }

        [[nodiscard]] constexpr auto operator==(const EntityId& other) const noexcept -> bool {
            return high == other.high && low == other.low;
        }

        [[nodiscard]] constexpr auto operator!=(const EntityId& other) const noexcept -> bool {
            return !(*this == other);
        }
    };

    struct EntityIdHash {
        [[nodiscard]] auto operator()(EntityId id) const noexcept -> std::size_t {
            const auto mixed =
                id.high ^
                (id.low +
                 0x9e3779b97f4a7c15ULL +
                 (id.high << 6U) +
                 (id.high >> 2U));

            return static_cast<std::size_t>(mixed);
        }
    };

    [[nodiscard]] inline auto parseEntityId(std::string_view value) -> std::optional<EntityId> {
        if (value.size() != 32) {
            return std::nullopt;
        }

        EntityId id{};
        const auto highResult = std::from_chars(value.data(), value.data() + 16, id.high, 16);
        auto lowResult = std::from_chars(value.data() + 16, value.data() + value.size(), id.low, 16);
        if (highResult.ec != std::errc{} || highResult.ptr != value.data() + 16 ||
            lowResult.ec != std::errc{} || lowResult.ptr != value.data() + value.size() ||
            !id.isValid()) {
            return std::nullopt;
        }
        return id;
    }

    [[nodiscard]] inline auto toString(EntityId id) -> std::string {
        constexpr char HexDigits[] = "0123456789abcdef";
        std::string result(32, '0');
        for (std::size_t index = 0; index < 16; ++index) {
            const auto shift = static_cast<unsigned>((15 - index) * 4);
            result[index] = HexDigits[(id.high >> shift) & 0x0fU];
            result[index + 16] = HexDigits[(id.low >> shift) & 0x0fU];
        }
        return result;
    }

    [[nodiscard]] inline auto generateProcessId() -> std::uint64_t {
        std::random_device randomDevice;

        std::uint64_t value =
            (static_cast<std::uint64_t>(randomDevice()) << 32U) ^
             static_cast<std::uint64_t>(randomDevice());

        value ^=
            static_cast<std::uint64_t>(
                std::chrono::high_resolution_clock::now()
                    .time_since_epoch()
                    .count());

        if (value == 0) {
            value = 1;
        }

        return value;
    }

    inline const std::uint64_t entityIdProcess = generateProcessId();
    inline std::atomic<std::uint64_t> entityIdCounter{1};

    [[nodiscard]] inline auto generateEntityId() noexcept -> EntityId {
        return EntityId{
            .high = entityIdProcess,
            .low = entityIdCounter.fetch_add(1, std::memory_order_relaxed)
        };
    }
}

#endif //SMB_ENTITYID_HPP
