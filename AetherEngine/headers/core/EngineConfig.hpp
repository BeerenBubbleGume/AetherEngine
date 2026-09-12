#ifndef AETHERENGINE_CORE_ENGINECONFIG_HPP
#define AETHERENGINE_CORE_ENGINECONFIG_HPP

#include <expected>
#include <filesystem>
#include <string>

namespace AetherEngine::core {
    struct EngineError {
        int code;
        std::string message;
    };

    struct EngineInitConfig {
        // Empty for a packaged game; an explicit project root for authoring.
        std::filesystem::path projectRoot;
        std::string startupScene{"Main"};
    };

    struct EnginePaths {
        // Empty in packaged mode. Resolved once at startup, independent of
        // subsequent changes to the working directory.
        std::filesystem::path projectRoot;
        std::filesystem::path assetsRoot;
        std::filesystem::path shadersRoot;
        std::filesystem::path scenesRoot;
    };

    [[nodiscard]] auto resolveEnginePaths(
        const EngineInitConfig& config,
        const std::filesystem::path& executableRoot
    ) -> std::expected<EnginePaths, EngineError>;
}

#endif
