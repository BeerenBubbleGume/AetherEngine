//
// Created by drhaz on 21.06.2026.
//

#ifndef SMB_GPROGRAM_HPP
#define SMB_GPROGRAM_HPP
#include <memory>
#include <bgfx/bgfx.h>

namespace engine::systems { class SResourceManager; }

namespace engine::graphics {
    class GProgram {
    public:
        friend class engine::systems::SResourceManager;
        struct GProgramDeleter {
            void operator()(GProgram* program) const {
                delete program;
            }
        };
        using GProgramPtr = std::shared_ptr<GProgram>;
        GProgram(const GProgram&) = delete;
        GProgram& operator=(const GProgram&) = delete;
        GProgram(GProgram&& other) noexcept;
        GProgram& operator=(GProgram&& other) noexcept;

        bgfx::ProgramHandle handle() const;

    private:
        explicit GProgram(bgfx::ProgramHandle handle);
        ~GProgram();
        bgfx::ProgramHandle m_program = BGFX_INVALID_HANDLE;
    };
}

#endif //SMB_GPROGRAM_HPP
