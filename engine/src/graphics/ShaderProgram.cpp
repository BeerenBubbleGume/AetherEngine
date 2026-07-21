//
// Created by drhaz on 21.06.2026.
//

#include "graphics/ShaderProgram.hpp"


namespace engine::graphics {
    ShaderProgram::ShaderProgram(bgfx::ProgramHandle handle) : m_program(handle) {}

    ShaderProgram::~ShaderProgram() {
        if (bgfx::isValid(m_program)) {
            bgfx::destroy(m_program);
        }
    }

    ShaderProgram::ShaderProgram(ShaderProgram &&other) noexcept {
        this->m_program = other.m_program;
        other.m_program = BGFX_INVALID_HANDLE;
    }

    ShaderProgram & ShaderProgram::operator=(ShaderProgram &&other) noexcept {
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

    bgfx::ProgramHandle ShaderProgram::handle() const {
        return m_program;
    }
} // graphics
// engine
