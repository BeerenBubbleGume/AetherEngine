//
// Created by Даниил Хазиев on 24.11.2026.
//

#ifndef AETHER_EDITOR_BGFXSHADERHELPER_HPP
#define AETHER_EDITOR_BGFXSHADERHELPER_HPP

#include <inttypes.h>
#include <bgfx/bgfx.h>
#include <imgui.h>

#if defined(_WIN32)
#include "imgui_vs_dx11.bin.h"
#include "imgui_fs_dx11.bin.h"
#include "collider_outline_vs_dx11.bin.h"
#include "collider_outline_fs_dx11.bin.h"
#elif defined(__APPLE__)
#include "imgui_vs_mtl.bin.h"
#include "imgui_fs_mtl.bin.h"
#include "collider_outline_vs_mtl.bin.h"
#include "collider_outline_fs_mtl.bin.h"
#endif
#include "imgui_vs_essl.bin.h"
#include "imgui_fs_essl.bin.h"
#include "collider_outline_vs_essl.bin.h"
#include "collider_outline_fs_essl.bin.h"
#include "imgui_vs_glsl.bin.h"
#include "imgui_fs_glsl.bin.h"
#include "collider_outline_vs_glsl.bin.h"
#include "collider_outline_fs_glsl.bin.h"
#include "imgui_vs_spv.bin.h"
#include "imgui_fs_spv.bin.h"
#include "collider_outline_vs_spv.bin.h"
#include "collider_outline_fs_spv.bin.h"

namespace AetherEditor::utils {
    struct ImGuiBgfxRendererState final {
        bgfx::Encoder* encoder{nullptr};
        bgfx::ViewId viewId{0};
        uint32_t samplerFlags{0};
    };
    struct ShaderBinary {
        const uint8_t* data{nullptr};
        uint32_t size{0};

        [[nodiscard]] auto isValid() const -> bool {
            return data != nullptr && size > 0;
        }
    };

    template<typename T, size_t Size>
    constexpr auto shaderBinary(const T (&data)[Size]) -> ShaderBinary {
        return {reinterpret_cast<const uint8_t*>(data), static_cast<uint32_t>(sizeof(data))};
    }

    struct ShaderPair {
        ShaderBinary vertex;
        ShaderBinary fragment;
    };

    inline auto imguiShadersFor(bgfx::RendererType::Enum renderer) -> ShaderPair {
        switch (renderer) {
#if defined(_WIN32)
            case bgfx::RendererType::Direct3D11:
            case bgfx::RendererType::Direct3D12:
                return {shaderBinary(imgui_vs_dx11), shaderBinary(imgui_fs_dx11)};
#elif defined(__APPLE__)
            case bgfx::RendererType::Metal:
                return {shaderBinary(imgui_vs_mtl), shaderBinary(imgui_fs_mtl)};
#endif
            case bgfx::RendererType::OpenGLES:
                return {shaderBinary(imgui_vs_essl), shaderBinary(imgui_fs_essl)};
            case bgfx::RendererType::OpenGL:
                return {shaderBinary(imgui_vs_glsl), shaderBinary(imgui_fs_glsl)};
            case bgfx::RendererType::Vulkan:
                return {shaderBinary(imgui_vs_spv), shaderBinary(imgui_fs_spv)};
            default: return {};
        }
    }

    inline auto colliderOutlineShadersFor(bgfx::RendererType::Enum renderer) -> ShaderPair {
        switch (renderer) {
#if defined(_WIN32)
            case bgfx::RendererType::Direct3D11:
            case bgfx::RendererType::Direct3D12:
                return {shaderBinary(collider_outline_vs_dx11), shaderBinary(collider_outline_fs_dx11)};
#elif defined(__APPLE__)
            case bgfx::RendererType::Metal:
                return {shaderBinary(collider_outline_vs_mtl), shaderBinary(collider_outline_fs_mtl)};
#endif
            case bgfx::RendererType::OpenGLES:
                return {shaderBinary(collider_outline_vs_essl), shaderBinary(collider_outline_fs_essl)};
            case bgfx::RendererType::OpenGL:
                return {shaderBinary(collider_outline_vs_glsl), shaderBinary(collider_outline_fs_glsl)};
            case bgfx::RendererType::Vulkan:
                return {shaderBinary(collider_outline_vs_spv), shaderBinary(collider_outline_fs_spv)};
            default: return {};
        }
    }

    template<typename Handle> auto destroyIfValid(Handle& handle) -> void {
        if (bgfx::isValid(handle)) {
            bgfx::destroy(handle);
            handle = BGFX_INVALID_HANDLE;
        }
    }

    constexpr uint32_t ImGuiSamplerLinear =
            BGFX_SAMPLER_U_CLAMP |
            BGFX_SAMPLER_V_CLAMP |
            BGFX_SAMPLER_W_CLAMP;

    constexpr uint32_t ImGuiSamplerNearest =
            ImGuiSamplerLinear |
            BGFX_SAMPLER_MIN_POINT |
            BGFX_SAMPLER_MAG_POINT |
            BGFX_SAMPLER_MIP_POINT;

    inline auto currentRendererState() -> ImGuiBgfxRendererState* {
        if (ImGui::GetCurrentContext() == nullptr) {
            return nullptr;
        }
        return static_cast<ImGuiBgfxRendererState*>(ImGui::GetPlatformIO().Renderer_RenderState);
    }

    inline auto resetRenderStateCallback(const ImDrawList*, const ImDrawCmd*) -> void {
        if (auto* state = currentRendererState()) {
            state->samplerFlags = ImGuiSamplerLinear;
        }
    }

    inline auto setSamplerLinearCallback(const ImDrawList*, const ImDrawCmd*) -> void {
        if (auto* state = currentRendererState()) {
            state->samplerFlags = ImGuiSamplerLinear;
        }
    }

    inline auto setSamplerNearestCallback(const ImDrawList*, const ImDrawCmd*) -> void {
        if (auto* state = currentRendererState()) {
            state->samplerFlags = ImGuiSamplerNearest;
        }
    }

    inline auto textureId(bgfx::TextureHandle texture) -> ImTextureID {
        if (!bgfx::isValid(texture)) {
            return ImTextureID_Invalid;
        }
        return static_cast<ImTextureID>(texture.idx) + 1;
    }

    inline auto textureHandle(ImTextureID id) -> bgfx::TextureHandle {
        if (id == ImTextureID_Invalid || id > static_cast<ImTextureID>(std::numeric_limits<uint16_t>::max())) {
            return BGFX_INVALID_HANDLE;
        }
        return {static_cast<uint16_t>(id - 1)};
    }

    constexpr uint64_t ImGuiRenderState =
        BGFX_STATE_WRITE_RGB |
        BGFX_STATE_WRITE_A |
        BGFX_STATE_DEPTH_TEST_ALWAYS |
        BGFX_STATE_MSAA |
        BGFX_STATE_BLEND_FUNC(
            BGFX_STATE_BLEND_SRC_ALPHA,
            BGFX_STATE_BLEND_INV_SRC_ALPHA
        );

    constexpr int MaxImGuiCommandLists = 1024;
    constexpr int MaxImGuiVerticesPerList = 1'000'000;
    constexpr int MaxImGuiIndicesPerList = 2'000'000;
    constexpr int MaxImGuiCommandsPerList = 100'000;
    constexpr std::size_t MaxImGuiCommandsPerFrame = 100'000;
    constexpr int MaxImGuiTextureUpdatesPerFrame = 4096;
    constexpr std::uint64_t MaxImGuiTextureBytes = 64ULL * 1024ULL * 1024ULL;
}

#endif //AETHER_EDITOR_BGFXSHADERHELPER_HPP