#include "assets/PlatformProfile.hpp"

namespace AetherEngine::assets {
    auto detectPlatformProfile(bgfx::RendererType::Enum renderer)
        -> std::optional<PlatformProfile> {
        PlatformProfile profile{};

#if defined(_WIN32)
        profile.os = OperatingSystem::Windows;
#elif defined(__APPLE__)
        profile.os = OperatingSystem::MacOS;
#elif defined(__linux__)
        profile.os = OperatingSystem::Linux;
#else
        return std::nullopt;
#endif

#if defined(_M_X64) || defined(__x86_64__)
        profile.architecture = Architecture::X64;
#elif defined(_M_ARM64) || defined(__aarch64__)
        profile.architecture = Architecture::Arm64;
#else
        return std::nullopt;
#endif

        switch (renderer) {
            case bgfx::RendererType::Direct3D11: profile.graphics = GraphicsBackend::Direct3D11; break;
            case bgfx::RendererType::Direct3D12: profile.graphics = GraphicsBackend::Direct3D12; break;
            case bgfx::RendererType::Vulkan: profile.graphics = GraphicsBackend::Vulkan; break;
            case bgfx::RendererType::Metal: profile.graphics = GraphicsBackend::Metal; break;
            case bgfx::RendererType::OpenGL: profile.graphics = GraphicsBackend::OpenGL; break;
            case bgfx::RendererType::OpenGLES: profile.graphics = GraphicsBackend::OpenGLES; break;
            default: return std::nullopt;
        }
        return profile;
    }
}
