//
// Created by drhaz on 21.06.2026.
//

#ifndef SMB_GPROGRAM_HPP
#define SMB_GPROGRAM_HPP
#include <memory>
#include <bgfx/bgfx.h>

namespace engine::resources { class RResourceManager; }

namespace engine::graphics {
    class GProgram {
    public:
        friend class engine::resources::RResourceManager;
        GProgram() = default;
        ~GProgram();

        GProgram(const GProgram&) = delete;
        GProgram& operator=(const GProgram&) = delete;
        GProgram(GProgram&& other) noexcept;
        GProgram& operator=(GProgram&& other) noexcept;

        [[nodiscard]] bgfx::ProgramHandle handle() const;

    private:
        explicit GProgram(bgfx::ProgramHandle handle);
        bgfx::ProgramHandle m_program = BGFX_INVALID_HANDLE;
    };
}

#endif //SMB_GPROGRAM_HPP
