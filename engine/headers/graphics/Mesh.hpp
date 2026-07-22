//
// Created by drhaz on 20.06.2026.
//

#ifndef SMB_GRAPHICS_MESH_HPP
#define SMB_GRAPHICS_MESH_HPP

#include <expected>
#include <span>
#include <string>
#include <bgfx/bgfx.h>
#include <vector>
#include <bx/math.h>
#include <bx/bounds.h>
#include <bx/readerwriter.h>

#include "math/Types.hpp"

namespace engine::resources { class ResourceManager; }

namespace engine::graphics {
    using Transform = engine::math::Transform;
    struct MeshError {
        int code;
        std::string message;
    };
    struct MeshGroup {
        bgfx::VertexBufferHandle    vbh = BGFX_INVALID_HANDLE;
        bgfx::IndexBufferHandle     ibh = BGFX_INVALID_HANDLE;
    };
    class Mesh {
    public:
        friend class engine::resources::ResourceManager;
        Mesh() = default;
        ~Mesh();

        Mesh(const Mesh&) = delete;
        Mesh(Mesh&&) noexcept;
        Mesh& operator=(const Mesh&) = delete;
        Mesh& operator=(Mesh&&) noexcept;

        void createFromVertices(
            std::span<const engine::math::Vec3> vertices,
            std::span<const uint16_t> indices
        );

        [[nodiscard]] auto loadFromBgfxGeometry(
            std::span<const std::uint8_t> bytes,
            std::size_t maximumGroups
        )
            -> std::expected<void, MeshError>;
        [[nodiscard]] auto submit(
            bgfx::ProgramHandle program,
            bgfx::UniformHandle colorUniform,
            bgfx::UniformHandle samplerUniform,
            bgfx::TextureHandle texture,
            const Transform &transform,
            math::Color baseColor,
            bgfx::ViewId viewId,
            uint64_t stage,
            std::size_t maximumGroups
        ) const -> std::size_t;
        [[nodiscard]] auto isValid() const -> bool;
    private:
        auto destroyHandles() -> void;

        std::vector<MeshGroup>     m_groups;
        bgfx::VertexLayout          m_layout;
    };
} // graphics

#endif //SMB_GRAPHICS_MESH_HPP
