#ifndef SMB_SECURITY_PATHSECURITY_HPP
#define SMB_SECURITY_PATHSECURITY_HPP

#include <expected>
#include <cstdint>
#include <filesystem>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace AetherEngine::security {
    struct PathSecurityError {
        std::string message;
    };

    struct SecureFileContents {
        std::filesystem::path canonicalPath;
        std::vector<std::uint8_t> bytes;
    };

    [[nodiscard]] auto canonicalDirectory(const std::filesystem::path& directory)
        -> std::expected<std::filesystem::path, PathSecurityError>;

    // Pure lexical validation/normalization. It performs no filesystem access
    // and is suitable for cache keys and trusted-manifest matching.
    [[nodiscard]] auto normalizeRelativePathWithin(
        const std::filesystem::path& canonicalRoot,
        std::string_view requestedPath
    ) -> std::expected<std::filesystem::path, PathSecurityError>;

    [[nodiscard]] auto resolveRegularFileWithin(
        const std::filesystem::path& canonicalRoot,
        std::string_view requestedPath
    ) -> std::expected<std::filesystem::path, PathSecurityError>;

    [[nodiscard]] auto resolveDirectoryWithin(
        const std::filesystem::path& canonicalRoot,
        std::string_view requestedPath
    ) -> std::expected<std::filesystem::path, PathSecurityError>;

    // Opens and reads the file while holding non-reparse directory handles for
    // the complete path. This closes the check/open race that a path-only API
    // cannot prevent when a local directory can be replaced concurrently.
    [[nodiscard]] auto readRegularFileWithin(
        const std::filesystem::path& canonicalRoot,
        std::string_view requestedPath,
        std::uint64_t maximumBytes
    ) -> std::expected<SecureFileContents, PathSecurityError>;

    // Writes through an OS handle whose final path and type were checked while
    // the non-reparse directory chain remains locked. POSIX uses an anchored
    // same-directory temporary file and atomic rename.
    [[nodiscard]] auto writeRegularFileWithin(
        const std::filesystem::path& canonicalRoot,
        std::string_view requestedPath,
        std::span<const std::uint8_t> contents,
        std::uint64_t maximumBytes
    ) -> std::expected<void, PathSecurityError>;

    [[nodiscard]] auto isSafeSceneName(std::string_view sceneName) -> bool;
}

#endif // SMB_SECURITY_PATHSECURITY_HPP
