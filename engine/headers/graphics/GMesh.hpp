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
#include <glm/fwd.hpp>
#include <glm/vec3.hpp>
#include <glm/detail/type_quat.hpp>
#include <bx/bounds.h>
#include <bx/readerwriter.h>
#include <fstream>
#include <vector>
#include <glm/gtc/type_ptr.hpp>

namespace engine::resources { class RResourceManager; }

namespace engine::graphics {
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
    struct GTransform {
        glm::vec3 position = {0.0f, 0.0f, 0.0f};
        glm::quat rotation = {1, 0, 0, 0};  // в градусах (yaw, pitch, roll)
        glm::vec3 scale    = {1.0f, 1.0f, 1.0f};

        void reset() {
            position = {0, 0, 0};
            rotation = glm::quat{1, 0, 0, 0};
            scale = {1, 1, 1};
        }
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
