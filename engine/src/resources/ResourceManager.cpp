//
// Created by drhaz on 21.06.2026.
//

#include "resources/ResourceManager.hpp"

#include <iostream>


namespace engine::resources {
    ResourceManager::ResourceManagerPtr ResourceManager::createResourceManager() {
        return ResourceManagerPtr(new ResourceManager(), ResourceManagerDeleter{});
    }

    auto ResourceManager::loadMesh(std::string_view filename) -> MeshHandle {
        const std::string key{filename};

        if (m_meshCache.contains(key)) {
            return { m_meshCache.at(key) };
        }

        graphics::Mesh mesh;
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

    auto ResourceManager::loadProgram(std::string_view name, std::string_view vertexShaderFilename, std::string_view fragmentShaderFilename) -> ProgramHandle {
        const std::string key{name};

        if (m_programCache.contains(key)) {
            return { m_programCache.at(key) };
        }

        bgfx::ShaderHandle vsh = loadShaderBinary(vertexShaderFilename);
        bgfx::ShaderHandle fsh = loadShaderBinary(fragmentShaderFilename);
        if (!bgfx::isValid(vsh) || !bgfx::isValid(fsh)) {
            return {};
        }
        bgfx::ProgramHandle ph = bgfx::createProgram(vsh, fsh, true);
        if (bgfx::isValid(ph)) {
            graphics::ShaderProgram program(ph);
            m_programs.push_back(std::move(program));
            auto id = static_cast<uint32_t>(m_programs.size());
            m_programCache[key] = id;
            return { id };
        }
        return {};
    }

    auto ResourceManager::loadTexture(std::string_view filename) -> TextureHandle {
        const std::string key{filename};

        if (m_textureCache.contains(key)) {
            return { m_textureCache.at(key) };
        }

        bgfx::TextureHandle th = loadTextureBinary(key);
        if (!bgfx::isValid(th)) {
            return {};
        }

        graphics::Texture texture(th);
        m_textures.push_back(std::move(texture));
        auto id = static_cast<uint32_t>(m_textures.size());
        m_textureCache[key] = id;
        return { id };
    }

    auto ResourceManager::getTexture(TextureHandle handle) const -> const graphics::Texture * {
        if (handle.id > 0 && handle.id <= m_textures.size()) {
            return &m_textures[handle.id - 1];
        }
        return nullptr;
    }

    auto ResourceManager::getMesh(MeshHandle handle) const -> const graphics::Mesh* {
        if (handle.id > 0 && handle.id <= m_meshes.size()) {
            return &m_meshes[handle.id - 1];
        }
        return nullptr;
    }

    auto ResourceManager::getProgram(ProgramHandle handle) const -> const graphics::ShaderProgram* {
        if (handle.id > 0 && handle.id <= m_programs.size()) {
            return &m_programs[handle.id - 1];
        }
        return nullptr;
    }

    auto ResourceManager::loadShaderBinary(std::string_view filename) -> bgfx::ShaderHandle {
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

    auto ResourceManager::loadTextureBinary(std::string_view filename) -> bgfx::TextureHandle {
        std::ifstream file(std::string{filename}, std::ios::binary | std::ios::ate);
        if (!file.is_open()) {
            return BGFX_INVALID_HANDLE;
        }

        const auto size = file.tellg();
        if (size <= 0) {
            return BGFX_INVALID_HANDLE;
        }

        file.seekg(0, std::ios::beg);

        const bgfx::Memory* mem = bgfx::alloc(static_cast<uint32_t>(size));
        if (!file.read(reinterpret_cast<char*>(mem->data), size)) {
            return BGFX_INVALID_HANDLE;
        }

        bgfx::TextureInfo info{};
        const auto handle = bgfx::createTexture(
            mem,
            BGFX_TEXTURE_NONE | BGFX_SAMPLER_NONE,
            0,
            &info
        );

        if (!bgfx::isValid(handle)) {
            return BGFX_INVALID_HANDLE;
        }

        return handle;
    }
} // engine
