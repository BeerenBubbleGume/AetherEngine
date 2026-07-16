//
// Created by drhaz on 29.06.2026.
//

#include "graphics/GTexture.hpp"

engine::graphics::GTexture::~GTexture() {
    if (isValid()) {
        destroyHandle();
    }
}

engine::graphics::GTexture::GTexture(GTexture &&other) noexcept {
    m_handle = other.m_handle;
    m_format = other.m_format;
    m_flags = other.m_flags;
    m_width = other.m_width;
    m_height = other.m_height;

    other.m_handle = BGFX_INVALID_HANDLE;
    other.m_format = bgfx::TextureFormat::Count;
    other.m_flags = 0;
    other.m_width = 0;
    other.m_height = 0;
}

engine::graphics::GTexture & engine::graphics::GTexture::operator=(GTexture &&other) noexcept {
    if (this == &other) {
        return *this;
    }

    destroyHandle();

    m_handle = other.m_handle;
    m_format = other.m_format;
    m_flags = other.m_flags;
    m_width = other.m_width;
    m_height = other.m_height;

    other.m_handle = BGFX_INVALID_HANDLE;
    other.m_format = bgfx::TextureFormat::Count;
    other.m_flags = 0;
    other.m_width = 0;
    other.m_height = 0;

    return *this;
}

auto engine::graphics::GTexture::isValid() const -> bool {
    return bgfx::isValid(m_handle);
}

auto engine::graphics::GTexture::handle() const -> bgfx::TextureHandle {
    return m_handle;
}

auto engine::graphics::GTexture::width() const -> uint16_t {
    return m_width;
}

auto engine::graphics::GTexture::height() const -> uint16_t {
    return m_height;
}

engine::graphics::GTexture::GTexture(bgfx::TextureHandle handle) : m_handle(handle) {
}

auto engine::graphics::GTexture::destroyHandle() -> void {
    if (bgfx::isValid(m_handle)) {
        bgfx::destroy(m_handle);
    }
    m_handle = BGFX_INVALID_HANDLE;
}
