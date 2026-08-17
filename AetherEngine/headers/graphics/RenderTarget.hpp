#ifndef SMB_RENDERTARGET_HPP
#define SMB_RENDERTARGET_HPP

#include <cstdint>
#include <expected>
#include <string>

#include <bgfx/bgfx.h>

namespace AetherEngine::graphics {
    struct RenderExtent {
        uint16_t width{0};
        uint16_t height{0};

        bool operator==(const RenderExtent&) const = default;
    };

    struct RenderTargetError {
        int code;
        std::string message;
    };

    // Owns an offscreen framebuffer and its color/depth attachments.
    // The object must be destroyed before bgfx is shut down.
    class RenderTarget final {
    public:
        RenderTarget() = default;
        ~RenderTarget();

        RenderTarget(const RenderTarget&) = delete;
        auto operator=(const RenderTarget&) -> RenderTarget& = delete;

        RenderTarget(RenderTarget&& other) noexcept;
        auto operator=(RenderTarget&& other) noexcept -> RenderTarget&;

        [[nodiscard]] auto resize(RenderExtent extent) -> std::expected<void, RenderTargetError>;
        auto reset() -> void;

        [[nodiscard]] auto framebuffer() const -> bgfx::FrameBufferHandle;
        [[nodiscard]] auto colorTexture() const -> bgfx::TextureHandle;
        [[nodiscard]] auto depthTexture() const -> bgfx::TextureHandle;
        [[nodiscard]] auto extent() const -> RenderExtent;
        [[nodiscard]] auto isValid() const -> bool;

    private:
        auto destroy() -> void;

        RenderExtent m_extent{};
        bgfx::TextureHandle m_colorTexture = BGFX_INVALID_HANDLE;
        bgfx::TextureHandle m_depthTexture = BGFX_INVALID_HANDLE;
        bgfx::FrameBufferHandle m_framebuffer = BGFX_INVALID_HANDLE;
    };
}

#endif // SMB_RENDERTARGET_HPP
