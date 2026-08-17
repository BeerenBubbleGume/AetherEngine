//
// Created by drhaz on 21.06.2026.
//

#ifndef SMB_SHADERPROGRAM_HPP
#define SMB_SHADERPROGRAM_HPP
#include <memory>
#include <bgfx/bgfx.h>

namespace AetherEngine::resources { class ResourceManager; }

namespace AetherEngine::graphics {
    class ShaderProgram {
    public:
        friend class AetherEngine::resources::ResourceManager;
        ShaderProgram() = default;
        ~ShaderProgram();

        ShaderProgram(const ShaderProgram&) = delete;
        ShaderProgram& operator=(const ShaderProgram&) = delete;
        ShaderProgram(ShaderProgram&& other) noexcept;
        ShaderProgram& operator=(ShaderProgram&& other) noexcept;

        [[nodiscard]] bgfx::ProgramHandle handle() const;

    private:
        explicit ShaderProgram(bgfx::ProgramHandle handle);
        bgfx::ProgramHandle m_program = BGFX_INVALID_HANDLE;
    };
}

#endif //SMB_SHADERPROGRAM_HPP
