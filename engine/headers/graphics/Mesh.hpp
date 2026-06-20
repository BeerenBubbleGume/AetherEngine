//
// Created by drhaz on 20.06.2026.
//

#ifndef SMB_MESH_HPP
#define SMB_MESH_HPP

#include <memory>
#include <string>
#include <bgfx/bgfx.h>
#include <vector>


namespace engine::graphics {
    struct PosColorVertex {
        float x, y, z;
        uint32_t abgr;
    };
    struct GMeshError {
        int code;
        std::string message;
    };
    class GMesh {
    public:
        struct GMeshDeleter {
            void operator()(GMesh* mesh) const {
                delete mesh;
            }
        };
        using GMeshPtr = std::unique_ptr<GMesh, GMeshDeleter>;
        static GMeshPtr createMesh();

        GMesh(const GMesh&) = delete;
        GMesh(GMesh&&) noexcept;
        GMesh& operator=(const GMesh&) = delete;
        GMesh& operator=(GMesh&&) noexcept;

        void createTriangle();           // для теста
        // void createFromVertices(...); // позже

        void submit(bgfx::ProgramHandle program) const;
        [[nodiscard]] auto isValid() const -> bool;
    private:
        GMesh() = default;
        ~GMesh() = default;
        bgfx::VertexBufferHandle m_vbh = BGFX_INVALID_HANDLE;
        bgfx::IndexBufferHandle  m_ibh = BGFX_INVALID_HANDLE;
        bgfx::VertexLayout       m_layout;
    };
} // graphics

#endif //SMB_MESH_HPP
