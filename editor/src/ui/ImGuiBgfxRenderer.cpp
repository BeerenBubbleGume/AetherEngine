#include "ui/ImGuiBgfxRenderer.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>

#include <bx/math.h>

#if defined(_WIN32)
#include "imgui_fs_dx11.bin.h"
#include "imgui_fs_essl.bin.h"
#include "imgui_fs_glsl.bin.h"
#include "imgui_fs_spv.bin.h"
#include "imgui_vs_dx11.bin.h"
#include "imgui_vs_essl.bin.h"
#include "imgui_vs_glsl.bin.h"
#include "imgui_vs_spv.bin.h"
#elif defined(__APPLE__)
#include "imgui_fs_essl.bin.h"
#include "imgui_fs_glsl.bin.h"
#include "imgui_fs_mtl.bin.h"
#include "imgui_fs_spv.bin.h"
#include "imgui_vs_essl.bin.h"
#include "imgui_vs_glsl.bin.h"
#include "imgui_vs_mtl.bin.h"
#include "imgui_vs_spv.bin.h"
#else
#include "imgui_fs_essl.bin.h"
#include "imgui_fs_glsl.bin.h"
#include "imgui_fs_spv.bin.h"
#include "imgui_vs_essl.bin.h"
#include "imgui_vs_glsl.bin.h"
#include "imgui_vs_spv.bin.h"
#endif

namespace AetherEditor::ui {
    namespace {
        struct ShaderBinary {
            const uint8_t* data{nullptr};
            uint32_t size{0};

            [[nodiscard]] auto isValid() const -> bool {
                return data != nullptr && size > 0;
            }
        };

        template<typename T, size_t Size>
        constexpr auto shaderBinary(const T (&data)[Size]) -> ShaderBinary {
            return {
                reinterpret_cast<const uint8_t*>(data),
                static_cast<uint32_t>(sizeof(data))
            };
        }

        auto vertexShaderFor(bgfx::RendererType::Enum renderer) -> ShaderBinary {
            switch (renderer) {
#if defined(_WIN32)
                case bgfx::RendererType::Direct3D11:
                case bgfx::RendererType::Direct3D12: return shaderBinary(imgui_vs_dx11);
#elif defined(__APPLE__)
                case bgfx::RendererType::Metal: return shaderBinary(imgui_vs_mtl);
#endif
                case bgfx::RendererType::OpenGLES: return shaderBinary(imgui_vs_essl);
                case bgfx::RendererType::OpenGL: return shaderBinary(imgui_vs_glsl);
                case bgfx::RendererType::Vulkan: return shaderBinary(imgui_vs_spv);
                default: return {};
            }
        }

        auto fragmentShaderFor(bgfx::RendererType::Enum renderer) -> ShaderBinary {
            switch (renderer) {
#if defined(_WIN32)
                case bgfx::RendererType::Direct3D11:
                case bgfx::RendererType::Direct3D12: return shaderBinary(imgui_fs_dx11);
#elif defined(__APPLE__)
                case bgfx::RendererType::Metal: return shaderBinary(imgui_fs_mtl);
#endif
                case bgfx::RendererType::OpenGLES: return shaderBinary(imgui_fs_essl);
                case bgfx::RendererType::OpenGL: return shaderBinary(imgui_fs_glsl);
                case bgfx::RendererType::Vulkan: return shaderBinary(imgui_fs_spv);
                default: return {};
            }
        }

        template<typename Handle>
        auto destroyIfValid(Handle& handle) -> void {
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

        auto currentRendererState() -> ImGuiBgfxRendererState* {
            if (ImGui::GetCurrentContext() == nullptr) {
                return nullptr;
            }
            return static_cast<ImGuiBgfxRendererState*>(ImGui::GetPlatformIO().Renderer_RenderState);
        }

        auto resetRenderStateCallback(const ImDrawList*, const ImDrawCmd*) -> void {
            if (auto* state = currentRendererState()) {
                state->samplerFlags = ImGuiSamplerLinear;
            }
        }

        auto setSamplerLinearCallback(const ImDrawList*, const ImDrawCmd*) -> void {
            if (auto* state = currentRendererState()) {
                state->samplerFlags = ImGuiSamplerLinear;
            }
        }

        auto setSamplerNearestCallback(const ImDrawList*, const ImDrawCmd*) -> void {
            if (auto* state = currentRendererState()) {
                state->samplerFlags = ImGuiSamplerNearest;
            }
        }

        constexpr uint64_t ImGuiRenderState =
            BGFX_STATE_WRITE_RGB |
            BGFX_STATE_WRITE_A |
            // Equivalent to disabled depth testing for UI (always passes and
            // never writes depth), while avoiding a D3D11 debug-layer state
            // alias that trips bgfx's state-cache ref-count assertion.
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

    auto ImGuiBgfxRenderer::createRenderer() -> ImGuiBgfxRendererPtr {
        return ImGuiBgfxRendererPtr(new ImGuiBgfxRenderer(), ImGuiBgfxRendererDeleter{});
    }

    auto ImGuiBgfxRenderer::init() -> std::expected<void, core::EditorError> {
        if (m_initialized) {
            return {};
        }
        if (ImGui::GetCurrentContext() == nullptr) {
            return std::unexpected(core::EditorError{1, "ImGui context must exist before renderer initialization"});
        }

        auto& io = ImGui::GetIO();
        if (io.BackendRendererUserData != nullptr) {
            return std::unexpected(core::EditorError{2, "An ImGui renderer backend is already initialized"});
        }

        m_vertexLayout
            .begin()
            .add(bgfx::Attrib::Position, 2, bgfx::AttribType::Float)
            .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
            .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
            .end();

        static_assert(sizeof(ImDrawVert) == 20, "Unexpected ImDrawVert layout");
        static_assert(offsetof(ImDrawVert, pos) == 0, "Unexpected ImDrawVert::pos offset");
        static_assert(offsetof(ImDrawVert, uv) == 8, "Unexpected ImDrawVert::uv offset");
        static_assert(offsetof(ImDrawVert, col) == 16, "Unexpected ImDrawVert::col offset");

        const auto renderer = bgfx::getRendererType();
        const auto vertexBinary = vertexShaderFor(renderer);
        const auto fragmentBinary = fragmentShaderFor(renderer);
        if (!vertexBinary.isValid() || !fragmentBinary.isValid()) {
            return std::unexpected(core::EditorError{3, "The active bgfx renderer has no embedded ImGui shaders"});
        }

        auto vertexShader = bgfx::createShader(bgfx::makeRef(vertexBinary.data, vertexBinary.size));
        auto fragmentShader = bgfx::createShader(bgfx::makeRef(fragmentBinary.data, fragmentBinary.size));
        if (!bgfx::isValid(vertexShader) || !bgfx::isValid(fragmentShader)) {
            destroyIfValid(vertexShader);
            destroyIfValid(fragmentShader);
            return std::unexpected(core::EditorError{4, "Failed to create ImGui shaders"});
        }

        m_program = bgfx::createProgram(vertexShader, fragmentShader, false);
        destroyIfValid(vertexShader);
        destroyIfValid(fragmentShader);
        if (!bgfx::isValid(m_program)) {
            return std::unexpected(core::EditorError{5, "Failed to create ImGui shader program"});
        }

        m_sampler = bgfx::createUniform("s_imguiTexture", bgfx::UniformType::Sampler);
        if (!bgfx::isValid(m_sampler)) {
            destroyIfValid(m_program);
            return std::unexpected(core::EditorError{6, "Failed to create ImGui sampler uniform"});
        }

        io.Fonts->TexDesiredFormat = ImTextureFormat_RGBA32;
        io.BackendRendererName = "aether_imgui_bgfx";
        io.BackendRendererUserData = this;
        io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;
        io.BackendFlags |= ImGuiBackendFlags_RendererHasTextures;

        auto& platformIo = ImGui::GetPlatformIO();
        platformIo.Renderer_TextureMaxWidth = static_cast<int>(bgfx::getCaps()->limits.maxTextureSize);
        platformIo.Renderer_TextureMaxHeight = static_cast<int>(bgfx::getCaps()->limits.maxTextureSize);
        platformIo.DrawCallback_ResetRenderState = resetRenderStateCallback;
        platformIo.DrawCallback_SetSamplerLinear = setSamplerLinearCallback;
        platformIo.DrawCallback_SetSamplerNearest = setSamplerNearestCallback;

        m_initialized = true;
        return {};
    }

    auto ImGuiBgfxRenderer::shutdown() -> void {
        if (ImGui::GetCurrentContext() != nullptr) {
            auto& io = ImGui::GetIO();
            if (io.BackendRendererUserData == this) {
                for (ImTextureData* texture : ImGui::GetPlatformIO().Textures) {
                    if (texture != nullptr && texture->RefCount == 1) {
                        texture->SetStatus(ImTextureStatus_WantDestroy);
                        destroyTexture(*texture);
                    }
                }

                io.BackendFlags &= ~(
                    ImGuiBackendFlags_RendererHasVtxOffset |
                    ImGuiBackendFlags_RendererHasTextures
                );
                io.BackendRendererName = nullptr;
                io.BackendRendererUserData = nullptr;
                ImGui::GetPlatformIO().ClearRendererHandlers();
            }
        }

        destroyIfValid(m_sampler);
        destroyIfValid(m_program);
        m_initialized = false;
    }

    auto ImGuiBgfxRenderer::renderDrawData(
        ImDrawData* drawData,
        bgfx::ViewId viewId,
        engine::graphics::RenderExtent extent
    ) -> void {
        if (!m_initialized || drawData == nullptr) {
            return;
        }

        const double scaledWidth =
            static_cast<double>(drawData->DisplaySize.x) * drawData->FramebufferScale.x;
        const double scaledHeight =
            static_cast<double>(drawData->DisplaySize.y) * drawData->FramebufferScale.y;
        if (!std::isfinite(scaledWidth) || !std::isfinite(scaledHeight) ||
            scaledWidth < std::numeric_limits<int32_t>::min() ||
            scaledHeight < std::numeric_limits<int32_t>::min() ||
            scaledWidth > std::numeric_limits<int32_t>::max() ||
            scaledHeight > std::numeric_limits<int32_t>::max()) {
            return;
        }
        const auto framebufferWidth = static_cast<int32_t>(scaledWidth);
        const auto framebufferHeight = static_cast<int32_t>(scaledHeight);
        if (framebufferWidth <= 0 || framebufferHeight <= 0 || extent.width == 0 || extent.height == 0) {
            return;
        }
        if (!std::isfinite(drawData->DisplayPos.x) || !std::isfinite(drawData->DisplayPos.y) ||
            !std::isfinite(drawData->DisplaySize.x) || !std::isfinite(drawData->DisplaySize.y) ||
            !std::isfinite(drawData->FramebufferScale.x) || !std::isfinite(drawData->FramebufferScale.y) ||
            drawData->CmdListsCount < 0 ||
            drawData->CmdListsCount > MaxImGuiCommandLists ||
            drawData->CmdListsCount != drawData->CmdLists.Size ||
            (drawData->CmdListsCount > 0 && drawData->CmdLists.Data == nullptr)) {
            return;
        }

        processTextureUpdates(drawData);

        bgfx::setViewName(viewId, "ImGui");
        bgfx::setViewMode(viewId, bgfx::ViewMode::Sequential);
        bgfx::setViewFrameBuffer(viewId, BGFX_INVALID_HANDLE);
        bgfx::setViewRect(viewId, 0, 0, extent.width, extent.height);
        bgfx::setViewClear(viewId, BGFX_CLEAR_NONE);

        const auto* caps = bgfx::getCaps();
        const float left = drawData->DisplayPos.x;
        const float right = left + drawData->DisplaySize.x;
        const float top = drawData->DisplayPos.y;
        const float bottom = top + drawData->DisplaySize.y;
        float projection[16];
        bx::mtxOrtho(
            projection,
            left,
            right,
            bottom,
            top,
            0.0f,
            1000.0f,
            0.0f,
            caps->homogeneousDepth
        );
        bgfx::setViewTransform(viewId, nullptr, projection);

        const ImVec2 clipOffset = drawData->DisplayPos;
        const ImVec2 clipScale = drawData->FramebufferScale;
        const float clipLimitX = std::min(static_cast<float>(framebufferWidth), static_cast<float>(extent.width));
        const float clipLimitY = std::min(static_cast<float>(framebufferHeight), static_cast<float>(extent.height));

        bgfx::Encoder* encoder = bgfx::begin();
        if (encoder == nullptr) {
            return;
        }

        auto& platformIo = ImGui::GetPlatformIO();
        ImGuiBgfxRendererState rendererState{
            .encoder = encoder,
            .viewId = viewId,
            .samplerFlags = ImGuiSamplerLinear
        };
        platformIo.Renderer_RenderState = &rendererState;

        std::size_t submittedCommands = 0;
        for (int32_t listIndex = 0; listIndex < drawData->CmdListsCount; ++listIndex) {
            const ImDrawList* drawList = drawData->CmdLists[listIndex];
            if (drawList == nullptr ||
                drawList->VtxBuffer.Size <= 0 ||
                drawList->IdxBuffer.Size <= 0 ||
                drawList->VtxBuffer.Size > MaxImGuiVerticesPerList ||
                drawList->IdxBuffer.Size > MaxImGuiIndicesPerList ||
                drawList->CmdBuffer.Size < 0 ||
                drawList->CmdBuffer.Size > MaxImGuiCommandsPerList ||
                drawList->VtxBuffer.Data == nullptr ||
                drawList->IdxBuffer.Data == nullptr ||
                (drawList->CmdBuffer.Size > 0 && drawList->CmdBuffer.Data == nullptr)) {
                continue;
            }
            const auto vertexCount = static_cast<uint32_t>(drawList->VtxBuffer.Size);
            const auto indexCount = static_cast<uint32_t>(drawList->IdxBuffer.Size);

            bgfx::TransientVertexBuffer vertexBuffer;
            bgfx::TransientIndexBuffer indexBuffer;
            if (!bgfx::allocTransientBuffers(
                    &vertexBuffer,
                    m_vertexLayout,
                    vertexCount,
                    &indexBuffer,
                    indexCount,
                    sizeof(ImDrawIdx) == sizeof(uint32_t))) {
                break;
            }

            std::memcpy(
                vertexBuffer.data,
                drawList->VtxBuffer.Data,
                vertexCount * sizeof(ImDrawVert)
            );
            std::memcpy(
                indexBuffer.data,
                drawList->IdxBuffer.Data,
                indexCount * sizeof(ImDrawIdx)
            );

            for (const ImDrawCmd& command : drawList->CmdBuffer) {
                if (submittedCommands >= MaxImGuiCommandsPerFrame) {
                    break;
                }
                if (command.UserCallback != nullptr) {
                    command.UserCallback(drawList, &command);
                    ++submittedCommands;
                    continue;
                }
                if (command.ElemCount == 0 ||
                    command.VtxOffset >= vertexCount ||
                    command.IdxOffset > indexCount ||
                    command.ElemCount > indexCount - command.IdxOffset) {
                    continue;
                }

                const float clipMinX = (command.ClipRect.x - clipOffset.x) * clipScale.x;
                const float clipMinY = (command.ClipRect.y - clipOffset.y) * clipScale.y;
                const float clipMaxX = (command.ClipRect.z - clipOffset.x) * clipScale.x;
                const float clipMaxY = (command.ClipRect.w - clipOffset.y) * clipScale.y;
                if (!std::isfinite(clipMinX) || !std::isfinite(clipMinY) ||
                    !std::isfinite(clipMaxX) || !std::isfinite(clipMaxY)) {
                    continue;
                }
                if (clipMaxX <= 0.0f || clipMaxY <= 0.0f || clipMinX >= clipLimitX || clipMinY >= clipLimitY) {
                    continue;
                }

                const float clippedMinX = std::max(clipMinX, 0.0f);
                const float clippedMinY = std::max(clipMinY, 0.0f);
                const float clippedMaxX = std::min(clipMaxX, clipLimitX);
                const float clippedMaxY = std::min(clipMaxY, clipLimitY);
                if (clippedMaxX <= clippedMinX || clippedMaxY <= clippedMinY) {
                    continue;
                }

                const auto scissorX = static_cast<uint16_t>(clippedMinX);
                const auto scissorY = static_cast<uint16_t>(clippedMinY);
                const auto scissorWidth = static_cast<uint16_t>(std::ceil(clippedMaxX) - scissorX);
                const auto scissorHeight = static_cast<uint16_t>(std::ceil(clippedMaxY) - scissorY);

                const auto texture = textureHandle(command.GetTexID());
                if (!bgfx::isValid(texture)) {
                    continue;
                }

                encoder->setScissor(scissorX, scissorY, scissorWidth, scissorHeight);
                encoder->setState(ImGuiRenderState);
                encoder->setTexture(0, m_sampler, texture, rendererState.samplerFlags);
                encoder->setVertexBuffer(
                    0,
                    &vertexBuffer,
                    command.VtxOffset,
                    vertexCount - command.VtxOffset
                );
                encoder->setIndexBuffer(&indexBuffer, command.IdxOffset, command.ElemCount);
                encoder->submit(viewId, m_program);
                ++submittedCommands;
            }
            if (submittedCommands >= MaxImGuiCommandsPerFrame) {
                break;
            }
        }

        platformIo.Renderer_RenderState = nullptr;
        bgfx::end(encoder);
    }

    auto ImGuiBgfxRenderer::textureId(bgfx::TextureHandle texture) -> ImTextureID {
        if (!bgfx::isValid(texture)) {
            return ImTextureID_Invalid;
        }
        return static_cast<ImTextureID>(texture.idx) + 1;
    }

    auto ImGuiBgfxRenderer::textureHandle(ImTextureID id) -> bgfx::TextureHandle {
        if (id == ImTextureID_Invalid || id > static_cast<ImTextureID>(std::numeric_limits<uint16_t>::max())) {
            return BGFX_INVALID_HANDLE;
        }
        return {static_cast<uint16_t>(id - 1)};
    }

    ImGuiBgfxRenderer::~ImGuiBgfxRenderer() {
        shutdown();
    }

    auto ImGuiBgfxRenderer::processTextureUpdates(ImDrawData* drawData) -> void {
        if (drawData == nullptr || drawData->Textures == nullptr) {
            return;
        }

        if (drawData->Textures->Size < 0 ||
            drawData->Textures->Size > MaxImGuiTextureUpdatesPerFrame ||
            (drawData->Textures->Size > 0 && drawData->Textures->Data == nullptr)) {
            return;
        }

        for (ImTextureData* texture : *drawData->Textures) {
            if (texture == nullptr) {
                continue;
            }

            switch (texture->Status) {
                case ImTextureStatus_WantCreate:
                    createTexture(*texture);
                    break;
                case ImTextureStatus_WantUpdates:
                    updateTexture(*texture);
                    break;
                case ImTextureStatus_WantDestroy:
                    destroyTexture(*texture);
                    break;
                default:
                    break;
            }
        }
    }

    auto ImGuiBgfxRenderer::createTexture(ImTextureData& texture) -> bool {
        if (texture.Format != ImTextureFormat_RGBA32 ||
            texture.Width <= 0 || texture.Height <= 0 ||
            texture.Width > std::numeric_limits<uint16_t>::max() ||
            texture.Height > std::numeric_limits<uint16_t>::max() ||
            texture.BytesPerPixel != 4 ||
            texture.Pixels == nullptr) {
            return false;
        }

        const auto dataSize =
            static_cast<std::uint64_t>(texture.Width) *
            static_cast<std::uint64_t>(texture.Height) *
            static_cast<std::uint64_t>(texture.BytesPerPixel);
        const auto* caps = bgfx::getCaps();
        if (dataSize > MaxImGuiTextureBytes ||
            dataSize > std::numeric_limits<uint32_t>::max() ||
            caps == nullptr ||
            texture.Width > caps->limits.maxTextureSize ||
            texture.Height > caps->limits.maxTextureSize ||
            !bgfx::isTextureValid(
                1,
                false,
                1,
                bgfx::TextureFormat::RGBA8,
                ImGuiSamplerLinear
            )) {
            return false;
        }
        const auto* memory = bgfx::copy(
            texture.Pixels,
            static_cast<uint32_t>(dataSize)
        );
        const auto handle = bgfx::createTexture2D(
            static_cast<uint16_t>(texture.Width),
            static_cast<uint16_t>(texture.Height),
            false,
            1,
            bgfx::TextureFormat::RGBA8,
            ImGuiSamplerLinear,
            memory
        );
        if (!bgfx::isValid(handle)) {
            return false;
        }

        bgfx::setName(handle, "ImGui texture");
        texture.SetTexID(textureId(handle));
        texture.SetStatus(ImTextureStatus_OK);
        return true;
    }

    auto ImGuiBgfxRenderer::updateTexture(ImTextureData& texture) -> bool {
        if (texture.Format != ImTextureFormat_RGBA32 ||
            texture.Width <= 0 ||
            texture.Height <= 0 ||
            texture.Width > std::numeric_limits<uint16_t>::max() ||
            texture.Height > std::numeric_limits<uint16_t>::max() ||
            texture.BytesPerPixel != 4 ||
            texture.Pixels == nullptr) {
            return false;
        }

        const auto fullTextureBytes =
            static_cast<std::uint64_t>(texture.Width) *
            static_cast<std::uint64_t>(texture.Height) *
            static_cast<std::uint64_t>(texture.BytesPerPixel);
        if (fullTextureBytes > MaxImGuiTextureBytes ||
            texture.Updates.Size < 0 ||
            texture.Updates.Size > MaxImGuiTextureUpdatesPerFrame ||
            (texture.Updates.Size > 0 && texture.Updates.Data == nullptr)) {
            return false;
        }

        const auto handle = textureHandle(texture.GetTexID());
        if (!bgfx::isValid(handle)) {
            return false;
        }

        for (const ImTextureRect& update : texture.Updates) {
            if (update.w == 0 || update.h == 0) {
                continue;
            }
            const auto right = static_cast<std::uint32_t>(update.x) + update.w;
            const auto bottom = static_cast<std::uint32_t>(update.y) + update.h;
            if (right > static_cast<std::uint32_t>(texture.Width) ||
                bottom > static_cast<std::uint32_t>(texture.Height)) {
                return false;
            }

            const uint32_t rowSize = static_cast<uint32_t>(update.w) * static_cast<uint32_t>(texture.BytesPerPixel);
            const std::uint64_t dataSize64 =
                static_cast<std::uint64_t>(rowSize) * static_cast<std::uint64_t>(update.h);
            if (dataSize64 > std::numeric_limits<uint32_t>::max()) {
                return false;
            }
            const auto dataSize = static_cast<uint32_t>(dataSize64);
            const bgfx::Memory* memory = bgfx::alloc(dataSize);
            for (uint32_t row = 0; row < update.h; ++row) {
                std::memcpy(
                    memory->data + row * rowSize,
                    texture.GetPixelsAt(update.x, update.y + static_cast<int>(row)),
                    rowSize
                );
            }

            bgfx::updateTexture2D(
                handle,
                0,
                0,
                update.x,
                update.y,
                update.w,
                update.h,
                memory
            );
        }

        texture.SetStatus(ImTextureStatus_OK);
        return true;
    }

    auto ImGuiBgfxRenderer::destroyTexture(ImTextureData& texture) -> void {
        auto handle = textureHandle(texture.GetTexID());
        destroyIfValid(handle);
        texture.SetTexID(ImTextureID_Invalid);
        texture.SetStatus(ImTextureStatus_Destroyed);
    }
} // namespace AetherEditor::ui
