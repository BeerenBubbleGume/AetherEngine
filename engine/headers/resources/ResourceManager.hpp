//
// Created by drhaz on 21.06.2026.
//

#ifndef SMB_RESOURCEMANAGER_HPP
#define SMB_RESOURCEMANAGER_HPP
#include <memory>
#include <unordered_map>
#include <fstream>
#include <vector>

#include "graphics/Mesh.hpp"
#include "graphics/ShaderProgram.hpp"
#include "graphics/Texture.hpp"
#include "ResourceTypes.hpp"

namespace engine::resources {
    class ResourceManager final {
    public:
        ResourceManager(const ResourceManager&) = delete;
        ResourceManager& operator=(const ResourceManager&) = delete;
        struct ResourceManagerDeleter {
            void operator()(ResourceManager* resourceManager) const {
                delete resourceManager;
            }
        };
        using ResourceManagerPtr = std::unique_ptr<ResourceManager, ResourceManagerDeleter>;
        static ResourceManagerPtr createResourceManager();

        [[nodiscard]] auto loadMesh(std::string_view filename) -> MeshHandle;
        [[nodiscard]] auto loadProgram(std::string_view name, std::string_view vertexShaderFilename, std::string_view fragmentShaderFilename) -> ProgramHandle;
        [[nodiscard]] auto loadTexture(std::string_view filename) -> TextureHandle;

        [[nodiscard]] auto getMesh(MeshHandle handle) const -> const graphics::Mesh*;
        [[nodiscard]] auto getProgram(ProgramHandle handle) const -> const graphics::ShaderProgram*;
        [[nodiscard]] auto getTexture(TextureHandle handle) const -> const graphics::Texture*;
    private:
        static auto loadShaderBinary(std::string_view filename) -> bgfx::ShaderHandle;
        static auto loadTextureBinary(std::string_view filename) -> bgfx::TextureHandle;

        ResourceManager() = default;
        ~ResourceManager() = default;

        std::vector<graphics::Mesh> m_meshes;
        std::unordered_map<std::string, uint32_t> m_meshCache;

        std::vector<graphics::ShaderProgram> m_programs;
        std::unordered_map<std::string, uint32_t> m_programCache;

        std::vector<graphics::Texture> m_textures;
        std::unordered_map<std::string, uint32_t> m_textureCache;
    };
} // systems
// engine

#endif //SMB_RESOURCEMANAGER_HPP
