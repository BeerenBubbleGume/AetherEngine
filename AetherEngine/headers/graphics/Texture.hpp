//
// Created by drhaz on 29.06.2026.
//

#ifndef SMB_TEXTURE_HPP
#define SMB_TEXTURE_HPP
#include <expected>
#include <string_view>
#include <bgfx/bgfx.h>

namespace AetherEngine::resources { class ResourceManager; }

namespace AetherEngine::graphics {
    class Texture {
    public:
        friend class AetherEngine::resources::ResourceManager;
        Texture() = default;
        ~Texture();

        Texture(const Texture&) = delete;
        Texture& operator=(const Texture&) = delete;
        Texture(Texture&& other) noexcept;
        Texture& operator=(Texture&& other) noexcept;

        [[nodiscard]] auto isValid() const -> bool;
        [[nodiscard]] auto handle() const -> bgfx::TextureHandle;

        [[nodiscard]] auto width() const -> uint16_t;
        [[nodiscard]] auto height() const -> uint16_t;
    private:
        explicit Texture(bgfx::TextureHandle handle);
        auto destroyHandle() -> void;

        bgfx::TextureHandle m_handle = BGFX_INVALID_HANDLE;

        uint16_t m_width{0};
        uint16_t m_height{0};
        bgfx::TextureFormat::Enum m_format{bgfx::TextureFormat::Count};
        uint64_t m_flags{0};
    };
}

#endif //SMB_TEXTURE_HPP
