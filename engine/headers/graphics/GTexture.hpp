//
// Created by drhaz on 29.06.2026.
//

#ifndef SMB_GTEXTURE_HPP
#define SMB_GTEXTURE_HPP
#include <expected>
#include <string_view>
#include <bgfx/bgfx.h>

namespace engine::resources { class RResourceManager; }

namespace engine::graphics {
    class GTexture {
    public:
        friend class engine::resources::RResourceManager;
        GTexture() = default;
        ~GTexture();

        GTexture(const GTexture&) = delete;
        GTexture& operator=(const GTexture&) = delete;
        GTexture(GTexture&& other) noexcept;
        GTexture& operator=(GTexture&& other) noexcept;

        [[nodiscard]] auto isValid() const -> bool;
        [[nodiscard]] auto handle() const -> bgfx::TextureHandle;

        [[nodiscard]] auto width() const -> uint16_t;
        [[nodiscard]] auto height() const -> uint16_t;
    private:
        explicit GTexture(bgfx::TextureHandle handle);
        auto destroyHandle() -> void;

        bgfx::TextureHandle m_handle = BGFX_INVALID_HANDLE;

        uint16_t m_width{0};
        uint16_t m_height{0};
        bgfx::TextureFormat::Enum m_format{bgfx::TextureFormat::Count};
        uint64_t m_flags{0};
    };
}

#endif //SMB_GTEXTURE_HPP
