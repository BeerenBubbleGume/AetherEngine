//
// Created by drhaz on 20.06.2026.
//

#ifndef SMB_MESH_HPP
#define SMB_MESH_HPP

#include <expected>
#include <memory>
#include <span>
#include <string>
#include <bgfx/bgfx.h>
#include <vector>
#include <bx/math.h>
#include <bx/bounds.h>
#include <bx/readerwriter.h>
#include <fstream>
#include <vector>

#include "math/UTypes.hpp"

namespace engine::resources { class RResourceManager; }

namespace engine::graphics {
    using GTransform = engine::math::GTransform;

    struct GVertex {
        float x, y, z;
        float nx, ny, nz;
    };
    struct GMeshError {
        int code;
        std::string message;
    };
    struct GMeshGroup {
        bgfx::VertexBufferHandle vbh = BGFX_INVALID_HANDLE;
        bgfx::IndexBufferHandle ibh = BGFX_INVALID_HANDLE;
    };
    class GMesh {
    public:
        friend class engine::resources::RResourceManager;
        GMesh() = default;
        ~GMesh();

        GMesh(const GMesh&) = delete;
        GMesh(GMesh&&) noexcept;
        GMesh& operator=(const GMesh&) = delete;
        GMesh& operator=(GMesh&&) noexcept;

        void createTriangle();           // для теста
        void createFromVertices(
            std::span<const GVertex> vertices,
            std::span<const uint16_t> indices
        );
        // void createFromVertices(...); // позже

        [[nodiscard]] auto loadFromBgfxGeometry(std::string_view filename) -> std::expected<void, GMeshError>;
        void submit(bgfx::ProgramHandle program, const GTransform &transform, uint8_t viewId) const;
        [[nodiscard]] auto isValid() const -> bool;
    private:
        auto destroyHandles() -> void;

        std::vector<GMeshGroup> m_groups;
        bgfx::VertexLayout       m_layout;
    };
} // graphics

#endif //SMB_MESH_HPP
