//
// Created by drhaz on 21.06.2026.
//

#ifndef SMB_SRESOURCEMANAGER_HPP
#define SMB_SRESOURCEMANAGER_HPP
#include <memory>
#include <unordered_map>
#include <fstream>

#include "graphics/GMesh.hpp"
#include "graphics/GProgram.hpp"


namespace engine::systems {
    class SResourceManager {
    public:
        SResourceManager(const SResourceManager&) = delete;
        SResourceManager& operator=(const SResourceManager&) = delete;
        struct SResourceManagerDeleter {
            void operator()(SResourceManager* resourceManager) const {
                delete resourceManager;
            }
        };
        using SResourceManagerPtr = std::unique_ptr<SResourceManager, SResourceManagerDeleter>;
        using MeshHandle = std::shared_ptr<graphics::GMesh>;
        using ProgramHandle = std::shared_ptr<graphics::GProgram>;
        static SResourceManagerPtr createResourceManager();

        [[nodiscard]] auto loadMesh(std::string_view filename) -> MeshHandle;
        [[nodiscard]] auto createTriangleMesh(std::string_view name) -> MeshHandle;
        [[nodiscard]] auto loadProgram(std::string_view name, std::string_view vertexShaderFilename, std::string_view fragmentShaderFilename) -> ProgramHandle;

        auto getMesh(std::string_view name) -> MeshHandle;
        auto getProgram(std::string_view name) -> ProgramHandle;

    private:

        static auto loadShaderBinary(std::string_view filename) -> bgfx::ShaderHandle;

        SResourceManager() = default;
        ~SResourceManager() = default;

        std::unordered_map<std::string, MeshHandle> m_meshes;
        std::unordered_map<std::string, ProgramHandle> m_programs;
    };
} // systems
// engine

#endif //SMB_SRESOURCEMANAGER_HPP
