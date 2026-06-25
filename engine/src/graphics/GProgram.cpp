//
// Created by drhaz on 21.06.2026.
//

#include "graphics/GProgram.hpp"


namespace engine::graphics {
    GProgram::GProgram(bgfx::ProgramHandle handle) : m_program(handle) {}

    GProgram::~GProgram() {
        if (bgfx::isValid(m_program)) {
            bgfx::destroy(m_program);
        }
    }

    GProgram::GProgram(GProgram &&other) noexcept {
        this->m_program = other.m_program;
        other.m_program = BGFX_INVALID_HANDLE;
    }

    GProgram & GProgram::operator=(GProgram &&other) noexcept {
        if (this == &other) {
            return *this;
        }
        if (bgfx::isValid(m_program)) {
            bgfx::destroy(m_program);
        }
        this->m_program = other.m_program;
        other.m_program = BGFX_INVALID_HANDLE;
        return *this;
    }

    bgfx::ProgramHandle GProgram::handle() const {
        return m_program;
    }
} // graphics
// engine
