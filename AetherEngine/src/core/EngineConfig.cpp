#include "core/EngineConfig.hpp"

#include "security/PathSecurity.hpp"

namespace AetherEngine::core {
    auto resolveEnginePaths(
        const EngineInitConfig& config,
        const std::filesystem::path& executableRoot
    ) -> std::expected<EnginePaths, EngineError> {
        if (!security::isSafeSceneName(config.startupScene)) {
            return std::unexpected(EngineError{2, "Startup scene must be a safe scene name"});
        }
        const auto& requestedRoot = config.projectRoot.empty() ? executableRoot : config.projectRoot;
        if (requestedRoot.empty()) {
            return std::unexpected(EngineError{2, "A project or executable directory is required"});
        }
        const auto root = security::canonicalDirectory(requestedRoot);
        if (!root) {
            return std::unexpected(EngineError{2, "Failed to open content root: " + root.error().message});
        }
        const auto assets = security::resolveDirectoryWithin(*root, "assets");
        if (!assets) {
            return std::unexpected(EngineError{2, "Failed to open project/runtime assets: " + assets.error().message});
        }
        const auto scenes = security::resolveDirectoryWithin(*assets, "scenes");
        if (!scenes) {
            return std::unexpected(EngineError{2, "Failed to open scenes: " + scenes.error().message});
        }
        const auto shaders = security::resolveDirectoryWithin(*assets, "shaders");
        if (!shaders) {
            return std::unexpected(EngineError{2, "Failed to open shaders: " + shaders.error().message});
        }
        return EnginePaths{
            .projectRoot = config.projectRoot.empty() ? std::filesystem::path{} : *root,
            .assetsRoot = *assets,
            .shadersRoot = *shaders,
            .scenesRoot = *scenes
        };
    }
}
