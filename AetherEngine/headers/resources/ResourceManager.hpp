//
// Created by drhaz on 21.06.2026.
//

#ifndef SMB_RESOURCEMANAGER_HPP
#define SMB_RESOURCEMANAGER_HPP

#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <span>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "graphics/Mesh.hpp"
#include "graphics/ShaderProgram.hpp"
#include "graphics/Texture.hpp"
#include "ResourceTypes.hpp"

namespace AetherEngine::resources {
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
        static ResourceManagerPtr createResourceManager(const std::filesystem::path& assetsRoot);

        [[nodiscard]] auto loadMesh(std::string_view filename) -> MeshHandle;
        [[nodiscard]] auto loadProgram(
            std::string_view name,
            std::string_view vertexShaderFilename,
            std::string_view fragmentShaderFilename
        ) -> ProgramHandle;
        [[nodiscard]] auto loadTexture(std::string_view filename) -> TextureHandle;

        [[nodiscard]] auto getMesh(MeshHandle handle) const -> const graphics::Mesh*;
        [[nodiscard]] auto getProgram(ProgramHandle handle) const -> const graphics::ShaderProgram*;
        [[nodiscard]] auto getTexture(TextureHandle handle) const -> const graphics::Texture*;

        auto releaseMesh(MeshHandle handle) -> void;
        auto releaseProgram(ProgramHandle handle) -> void;
        auto releaseTexture(TextureHandle handle) -> void;
    private:
        struct MeshSlot {
            std::optional<graphics::Mesh> object;
            std::string cacheKey;
            std::uint64_t sourceBytes{};
            std::size_t groupCount{};
            std::uint32_t referenceCount{};
        };

        struct ProgramSlot {
            std::optional<graphics::ShaderProgram> object;
            std::string cacheKey;
            std::uint64_t sourceBytes{};
            std::uint32_t referenceCount{};
        };

        struct TextureSlot {
            std::optional<graphics::Texture> object;
            std::string cacheKey;
            std::uint64_t sourceBytes{};
            std::uint32_t referenceCount{};
        };

        static auto loadTextureBinary(std::span<const std::uint8_t> bytes) -> bgfx::TextureHandle;

        explicit ResourceManager(std::filesystem::path assetsRoot);
        ~ResourceManager() = default;

        std::filesystem::path m_assetsRoot;
        std::uint64_t m_loadedSourceBytes{0};
        std::size_t m_loadedMeshGroups{0};

        std::vector<MeshSlot> m_meshes;
        std::unordered_map<std::string, uint32_t> m_meshCache;
        std::unordered_set<std::string> m_failedMeshes;

        std::vector<ProgramSlot> m_programs;
        std::unordered_map<std::string, uint32_t> m_programCache;
        std::unordered_set<std::string> m_failedPrograms;

        std::vector<TextureSlot> m_textures;
        std::unordered_map<std::string, uint32_t> m_textureCache;
        std::unordered_set<std::string> m_failedTextures;

        std::unordered_set<std::string> m_seenResourceRequests;
        std::unordered_set<std::string> m_failedMeshRequests;
        std::unordered_set<std::string> m_failedProgramRequests;
        std::unordered_set<std::string> m_failedTextureRequests;
        std::size_t m_rejectionLogCount{0};
    };
} // namespace AetherEngine::resources

#endif // SMB_RESOURCEMANAGER_HPP
