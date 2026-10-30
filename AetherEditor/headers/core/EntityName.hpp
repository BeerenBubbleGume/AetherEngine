#ifndef SMB_ENTITYNAME_HPP
#define SMB_ENTITYNAME_HPP

#include <cstddef>
#include <string_view>

namespace AetherEditor::core {
    inline constexpr std::size_t MaxEntityNameBytes = 1024;
    // SceneSerializer accepts entity names up to 1024 UTF-8 bytes.
    [[nodiscard]] inline auto isValidEntityName(std::string_view name) -> bool {
        return name.size() <= MaxEntityNameBytes &&
            name.find_first_not_of(" \t\r\n") != std::string_view::npos &&
            name.find('\0') == std::string_view::npos;
    }
}

#endif // SMB_ENTITYNAME_HPP
