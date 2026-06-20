//
// Created by drhaz on 21.06.2026.
//

#include "systems/SResourceManager.hpp"


namespace engine::systems {
    SResourceManager::SResourceManagerPtr SResourceManager::createResourceManager() {
        return SResourceManagerPtr(new SResourceManager(), SResourceManagerDeleter{});
    }

    auto SResourceManager::loadMesh(std::string_view filename) -> MeshHandle {
        if (m_meshes.contains(filename.data())) {
            return m_meshes[filename.data()];
        }
        // Пока не реализовано чтение из файла, возвращаем пустоту или создаем дефолтный
        return nullptr;
    }

    auto SResourceManager::createTriangleMesh(std::string_view name) -> MeshHandle {
        if (m_meshes.contains(name.data())) {
            return m_meshes[name.data()];
        }
        auto mesh = MeshHandle(new graphics::GMesh(), graphics::GMesh::GMeshDeleter{});
        mesh->createTriangle();
        m_meshes[name.data()] = mesh;
        return mesh;
    }

    auto SResourceManager::loadProgram(std::string_view name, std::string_view vertexShaderFilename, std::string_view fragmentShaderFilename) -> ProgramHandle {
        if (m_programs.contains(name.data())) {
            return m_programs[name.data()];
        }

        bgfx::ShaderHandle vsh = loadShaderBinary(vertexShaderFilename);
        bgfx::ShaderHandle fsh = loadShaderBinary(fragmentShaderFilename);

        bgfx::ProgramHandle ph = bgfx::createProgram(vsh, fsh, true);
        if (bgfx::isValid(ph)) {
            auto program = ProgramHandle(new graphics::GProgram(ph), graphics::GProgram::GProgramDeleter{});
            m_programs[name.data()] = program;
            return program;
        }
        return nullptr;
    }

    auto SResourceManager::getMesh(std::string_view name) -> MeshHandle {
        return m_meshes.contains(name.data()) ? m_meshes[name.data()] : nullptr;
    }

    auto SResourceManager::getProgram(std::string_view name) -> ProgramHandle {
        return m_programs.contains(name.data()) ? m_programs[name.data()] : nullptr;
    }

    auto SResourceManager::loadShaderBinary(std::string_view filename) -> bgfx::ShaderHandle {
        std::ifstream file(filename.data(), std::ios::binary | std::ios::ate);
        if (!file.is_open()) {
            return BGFX_INVALID_HANDLE;
        }

        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);

        const bgfx::Memory* mem = bgfx::alloc(static_cast<uint32_t>(size + 1));
        if (file.read(reinterpret_cast<char*>(mem->data), size)) {
            mem->data[mem->size - 1] = '\0';
            return bgfx::createShader(mem);
        }

        return BGFX_INVALID_HANDLE;
    }
} // systems
// engine