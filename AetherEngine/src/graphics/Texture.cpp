//
// Created by drhaz on 29.06.2026.
//

#include "graphics/Texture.hpp"

AetherEngine::graphics::Texture::~Texture() {
    if (isValid()) {
        destroyHandle();
    }
}

AetherEngine::graphics::Texture::Texture(Texture &&other) noexcept {
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

AetherEngine::graphics::Texture & AetherEngine::graphics::Texture::operator=(Texture &&other) noexcept {
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

auto AetherEngine::graphics::Texture::isValid() const -> bool {
    return bgfx::isValid(m_handle);
}

auto AetherEngine::graphics::Texture::handle() const -> bgfx::TextureHandle {
    return m_handle;
}

auto AetherEngine::graphics::Texture::width() const -> uint16_t {
    return m_width;
}

auto AetherEngine::graphics::Texture::height() const -> uint16_t {
    return m_height;
}

AetherEngine::graphics::Texture::Texture(bgfx::TextureHandle handle) : m_handle(handle) {
}

auto AetherEngine::graphics::Texture::destroyHandle() -> void {
    if (bgfx::isValid(m_handle)) {
        bgfx::destroy(m_handle);
    }
    m_handle = BGFX_INVALID_HANDLE;
}
