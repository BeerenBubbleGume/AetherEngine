//
// Created by drhaz on 20.06.2026.
//

#ifndef SMB_MESH_HPP
#define SMB_MESH_HPP

#include <memory>
#include <string>
#include <bgfx/bgfx.h>
#include <vector>
#include <bx/math.h>


namespace engine::systems { class SResourceManager; }

namespace engine::graphics {
    struct PosColorVertex {
        float x, y, z;
        uint32_t abgr;
    };
    struct GMeshError {
        int code;
        std::string message;
    };
    struct GTransform {
        bx::Vec3 position = {0.0f, 0.0f, 0.0f};
        bx::Vec3 rotation = {0.0f, 0.0f, 0.0f};  // в градусах (yaw, pitch, roll)
        bx::Vec3 scale    = {1.0f, 1.0f, 1.0f};

        void reset() {
            position = {0, 0, 0};
            rotation = {0, 0, 0};
            scale = {1, 1, 1};
        }
    };
    class GMesh {
    public:
        friend class engine::systems::SResourceManager;
        struct GMeshDeleter {
            void operator()(GMesh* mesh) const {
                delete mesh;
            }
        };
        using GMeshPtr = std::shared_ptr<GMesh>;
        static GMeshPtr createMesh();

        GMesh(const GMesh&) = delete;
        GMesh(GMesh&&) noexcept;
        GMesh& operator=(const GMesh&) = delete;
        GMesh& operator=(GMesh&&) noexcept;

        void createTriangle();           // для теста
        // void createFromVertices(...); // позже

        void submit(bgfx::ProgramHandle program, const GTransform &transform, uint8_t viewId) const;
        [[nodiscard]] auto isValid() const -> bool;
    private:
        GMesh() = default;
        ~GMesh();
        bgfx::VertexBufferHandle m_vbh = BGFX_INVALID_HANDLE;
        bgfx::IndexBufferHandle  m_ibh = BGFX_INVALID_HANDLE;
        bgfx::VertexLayout       m_layout;
    };
} // graphics

#endif //SMB_MESH_HPP
