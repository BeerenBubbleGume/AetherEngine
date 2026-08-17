#include "graphics/RenderTarget.hpp"

#include <utility>

namespace AetherEngine::graphics {
    RenderTarget::~RenderTarget() {
        destroy();
    }

    RenderTarget::RenderTarget(RenderTarget&& other) noexcept
        : m_extent(std::exchange(other.m_extent, RenderExtent{})),
          m_colorTexture(std::exchange(other.m_colorTexture, BGFX_INVALID_HANDLE)),
          m_depthTexture(std::exchange(other.m_depthTexture, BGFX_INVALID_HANDLE)),
          m_framebuffer(std::exchange(other.m_framebuffer, BGFX_INVALID_HANDLE)) {
    }

    auto RenderTarget::operator=(RenderTarget&& other) noexcept -> RenderTarget& {
        if (this == &other) {
            return *this;
        }

        destroy();
        m_extent = std::exchange(other.m_extent, RenderExtent{});
        m_colorTexture = std::exchange(other.m_colorTexture, BGFX_INVALID_HANDLE);
        m_depthTexture = std::exchange(other.m_depthTexture, BGFX_INVALID_HANDLE);
        m_framebuffer = std::exchange(other.m_framebuffer, BGFX_INVALID_HANDLE);
        return *this;
    }

    auto RenderTarget::resize(RenderExtent extent) -> std::expected<void, RenderTargetError> {
        if (extent.width == 0 || extent.height == 0) {
            return std::unexpected(RenderTargetError{1, "Render target dimensions must be greater than zero"});
        }

        if (isValid() && extent == m_extent) {
            return {};
        }

        const auto colorTexture = bgfx::createTexture2D(
            extent.width,
            extent.height,
            false,
            1,
            bgfx::TextureFormat::RGBA8,
            BGFX_TEXTURE_RT | BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP
        );
        if (!bgfx::isValid(colorTexture)) {
            return std::unexpected(RenderTargetError{2, "Failed to create render target color texture"});
        }

        const auto depthTexture = bgfx::createTexture2D(
            extent.width,
            extent.height,
            false,
            1,
            bgfx::TextureFormat::D24S8,
            BGFX_TEXTURE_RT_WRITE_ONLY
        );
        if (!bgfx::isValid(depthTexture)) {
            bgfx::destroy(colorTexture);
            return std::unexpected(RenderTargetError{3, "Failed to create render target depth texture"});
        }

        const bgfx::TextureHandle attachments[] = {colorTexture, depthTexture};
        const auto framebuffer = bgfx::createFrameBuffer(2, attachments, false);
        if (!bgfx::isValid(framebuffer)) {
            bgfx::destroy(depthTexture);
            bgfx::destroy(colorTexture);
            return std::unexpected(RenderTargetError{4, "Failed to create render target framebuffer"});
        }

        destroy();
        m_extent = extent;
        m_colorTexture = colorTexture;
        m_depthTexture = depthTexture;
        m_framebuffer = framebuffer;
        return {};
    }

    auto RenderTarget::reset() -> void {
        destroy();
    }

    auto RenderTarget::framebuffer() const -> bgfx::FrameBufferHandle {
        return m_framebuffer;
    }

    auto RenderTarget::colorTexture() const -> bgfx::TextureHandle {
        return m_colorTexture;
    }

    auto RenderTarget::depthTexture() const -> bgfx::TextureHandle {
        return m_depthTexture;
    }

    auto RenderTarget::extent() const -> RenderExtent {
        return m_extent;
    }

    auto RenderTarget::isValid() const -> bool {
        return bgfx::isValid(m_framebuffer) &&
               bgfx::isValid(m_colorTexture) &&
               bgfx::isValid(m_depthTexture);
    }

    auto RenderTarget::destroy() -> void {
        if (bgfx::isValid(m_framebuffer)) {
            bgfx::destroy(m_framebuffer);
            m_framebuffer = BGFX_INVALID_HANDLE;
        }
        if (bgfx::isValid(m_colorTexture)) {
            bgfx::destroy(m_colorTexture);
            m_colorTexture = BGFX_INVALID_HANDLE;
        }
        if (bgfx::isValid(m_depthTexture)) {
            bgfx::destroy(m_depthTexture);
            m_depthTexture = BGFX_INVALID_HANDLE;
        }
        m_extent = {};
    }
}
