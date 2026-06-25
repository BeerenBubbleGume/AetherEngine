//
// Created by drhaz on 21.06.2026.
//

#include "resources/RResourceManager.hpp"

#include <iostream>


namespace engine::resources {
    RResourceManager::SResourceManagerPtr RResourceManager::createResourceManager() {
        return SResourceManagerPtr(new RResourceManager(), SResourceManagerDeleter{});
    }

    auto RResourceManager::loadMesh(std::string_view filename) -> RMeshHandle {
        const std::string key{filename};

        if (auto it = m_meshCache.find(key); it != m_meshCache.end()) {
            return { it->second };
        }

        graphics::GMesh mesh;
        auto loadResult = mesh.loadFromBgfxGeometry(key);
        if (!loadResult) {
            std::cerr << "Failed to load mesh '" << key << "': " << loadResult.error().message << std::endl;
            return {};
        }

        if (!mesh.isValid()) {
            return {};
        }

        m_meshes.push_back(std::move(mesh));

        const auto id = static_cast<uint32_t>(m_meshes.size());
        m_meshCache[key] = id;

        return { id };
    }

    auto RResourceManager::createTriangleMesh(std::string_view name) -> RMeshHandle {
        const std::string key{name};

        if (auto it = m_meshCache.find(key); it != m_meshCache.end()) {
            return { it->second };
        }
        
        graphics::GMesh mesh;
        mesh.createTriangle();
        
        m_meshes.push_back(std::move(mesh));
        uint32_t id = static_cast<uint32_t>(m_meshes.size());
        m_meshCache[key] = id;
        
        return { id };
    }

    auto RResourceManager::loadProgram(std::string_view name, std::string_view vertexShaderFilename, std::string_view fragmentShaderFilename) -> RProgramHandle {
        const std::string key{name};

        if (auto it = m_programCache.find(key); it != m_programCache.end()) {
            return { it->second };
        }

        bgfx::ShaderHandle vsh = loadShaderBinary(vertexShaderFilename);
        bgfx::ShaderHandle fsh = loadShaderBinary(fragmentShaderFilename);
        if (!bgfx::isValid(vsh) || !bgfx::isValid(fsh)) {
            return {};
        }
        bgfx::ProgramHandle ph = bgfx::createProgram(vsh, fsh, true);
        if (bgfx::isValid(ph)) {
            graphics::GProgram program(ph);
            m_programs.push_back(std::move(program));
            uint32_t id = static_cast<uint32_t>(m_programs.size());
            m_programCache[key] = id;
            return { id };
        }
        return {};
    }

    auto RResourceManager::getMesh(RMeshHandle handle) const -> const graphics::GMesh* {
        if (handle.id > 0 && handle.id <= m_meshes.size()) {
            return &m_meshes[handle.id - 1];
        }
        return nullptr;
    }

    auto RResourceManager::getProgram(RProgramHandle handle) const -> const graphics::GProgram* {
        if (handle.id > 0 && handle.id <= m_programs.size()) {
            return &m_programs[handle.id - 1];
        }
        return nullptr;
    }

    auto RResourceManager::loadShaderBinary(std::string_view filename) -> bgfx::ShaderHandle {
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
} // engine
