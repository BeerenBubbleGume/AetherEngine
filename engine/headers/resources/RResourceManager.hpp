//
// Created by drhaz on 21.06.2026.
//

#ifndef SMB_SRESOURCEMANAGER_HPP
#define SMB_SRESOURCEMANAGER_HPP
#include <memory>
#include <unordered_map>
#include <fstream>
#include <vector>

#include "graphics/GMesh.hpp"
#include "graphics/GProgram.hpp"
#include "graphics/GTexture.hpp"
#include "RTypes.hpp"

namespace engine::resources {
    class RResourceManager final {
    public:
        RResourceManager(const RResourceManager&) = delete;
        RResourceManager& operator=(const RResourceManager&) = delete;
        struct SResourceManagerDeleter {
            void operator()(RResourceManager* resourceManager) const {
                delete resourceManager;
            }
        };
        using SResourceManagerPtr = std::unique_ptr<RResourceManager, SResourceManagerDeleter>;
        static SResourceManagerPtr createResourceManager();

        [[nodiscard]] auto loadMesh(std::string_view filename) -> RMeshHandle;
        [[nodiscard]] auto loadProgram(std::string_view name, std::string_view vertexShaderFilename, std::string_view fragmentShaderFilename) -> RProgramHandle;
        [[nodiscard]] auto loadTexture(std::string_view filename) -> RTextureHandle;

        [[nodiscard]] auto getMesh(RMeshHandle handle) const -> const graphics::GMesh*;
        [[nodiscard]] auto getProgram(RProgramHandle handle) const -> const graphics::GProgram*;
        [[nodiscard]] auto getTexture(RTextureHandle handle) const -> const graphics::GTexture*;
    private:
        static auto loadShaderBinary(std::string_view filename) -> bgfx::ShaderHandle;
        static auto loadTextureBinary(std::string_view filename) -> bgfx::TextureHandle;

        RResourceManager() = default;
        ~RResourceManager() = default;

        std::vector<graphics::GMesh> m_meshes;
        std::unordered_map<std::string, uint32_t> m_meshCache;

        std::vector<graphics::GProgram> m_programs;
        std::unordered_map<std::string, uint32_t> m_programCache;

        std::vector<graphics::GTexture> m_textures;
        std::unordered_map<std::string, uint32_t> m_textureCache;
    };
} // systems
// engine

#endif //SMB_SRESOURCEMANAGER_HPP
