#ifndef AETHERENGINE_ASSETS_PLATFORMPROFILE_HPP
#define AETHERENGINE_ASSETS_PLATFORMPROFILE_HPP

#include <optional>

#include <bgfx/bgfx.h>

namespace AetherEngine::assets {
    enum class OperatingSystem {
        Any,
        Windows,
        MacOS,
        Linux
    };

    enum class Architecture {
        Any,
        X64,
        Arm64
    };

    enum class GraphicsBackend {
        Any,
        Direct3D11,
        Direct3D12,
        Vulkan,
        Metal,
        OpenGL,
        OpenGLES
    };

    struct PlatformProfile {
        OperatingSystem os{OperatingSystem::Any};
        Architecture architecture{Architecture::Any};
        GraphicsBackend graphics{GraphicsBackend::Any};

        bool operator==(const PlatformProfile&) const = default;
    };

    [[nodiscard]] auto detectPlatformProfile(bgfx::RendererType::Enum renderer)
        -> std::optional<PlatformProfile>;
}

#endif // AETHERENGINE_ASSETS_PLATFORMPROFILE_HPP
