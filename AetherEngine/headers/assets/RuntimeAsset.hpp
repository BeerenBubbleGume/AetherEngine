#ifndef AETHERENGINE_ASSETS_RUNTIMEASSET_HPP
#define AETHERENGINE_ASSETS_RUNTIMEASSET_HPP

#include <cstddef>
#include <utility>
#include <vector>

#include "AssetTypes.hpp"
#include "resources/ResourceManager.hpp"

namespace AetherEngine::assets {
    class RuntimeAsset {
    public:
        RuntimeAsset() = default;
        RuntimeAsset(const RuntimeAsset&) = delete;
        auto operator=(const RuntimeAsset&) -> RuntimeAsset& = delete;
        virtual ~RuntimeAsset() = default;

        [[nodiscard]] virtual auto type() const noexcept -> AssetType = 0;
        [[nodiscard]] virtual auto residentBytes() const noexcept -> std::size_t {
            return 0;
        }
    };

    class RuntimeMeshAsset final : public RuntimeAsset {
    public:
        RuntimeMeshAsset(resources::ResourceManager& resources, resources::MeshHandle handle)
            : m_resources(&resources), m_handle(handle) {}
        ~RuntimeMeshAsset() override {
            if (m_resources) m_resources->releaseMesh(m_handle);
        }

        [[nodiscard]] auto type() const noexcept -> AssetType override { return AssetType::Mesh; }
        [[nodiscard]] auto mesh() const -> const graphics::Mesh* {
            return m_resources ? m_resources->getMesh(m_handle) : nullptr;
        }

    private:
        resources::ResourceManager* m_resources{};
        resources::MeshHandle m_handle{};
    };

    class RuntimeTextureAsset final : public RuntimeAsset {
    public:
        RuntimeTextureAsset(resources::ResourceManager& resources, resources::TextureHandle handle)
            : m_resources(&resources), m_handle(handle) {}
        ~RuntimeTextureAsset() override {
            if (m_resources) m_resources->releaseTexture(m_handle);
        }

        [[nodiscard]] auto type() const noexcept -> AssetType override { return AssetType::Texture; }
        [[nodiscard]] auto texture() const -> const graphics::Texture* {
            return m_resources ? m_resources->getTexture(m_handle) : nullptr;
        }

    private:
        resources::ResourceManager* m_resources{};
        resources::TextureHandle m_handle{};
    };

    class RuntimeShaderAsset final : public RuntimeAsset {
    public:
        RuntimeShaderAsset(resources::ResourceManager& resources, resources::ProgramHandle handle)
            : m_resources(&resources), m_handle(handle) {}
        ~RuntimeShaderAsset() override {
            if (m_resources) m_resources->releaseProgram(m_handle);
        }

        [[nodiscard]] auto type() const noexcept -> AssetType override { return AssetType::Shader; }
        [[nodiscard]] auto program() const -> const graphics::ShaderProgram* {
            return m_resources ? m_resources->getProgram(m_handle) : nullptr;
        }

    private:
        resources::ResourceManager* m_resources{};
        resources::ProgramHandle m_handle{};
    };

    class RuntimeMaterialAsset final : public RuntimeAsset {
    public:
        RuntimeMaterialAsset(
            AssetHandle<RuntimeShaderAsset> shader,
            std::vector<AssetHandle<RuntimeTextureAsset>> textures,
            resources::RenderState renderState
        ) : m_shader(shader), m_textures(std::move(textures)), m_renderState(renderState) {}

        [[nodiscard]] auto type() const noexcept -> AssetType override { return AssetType::Material; }
        [[nodiscard]] auto shader() const noexcept -> AssetHandle<RuntimeShaderAsset> { return m_shader; }
        [[nodiscard]] auto textures() const noexcept
            -> const std::vector<AssetHandle<RuntimeTextureAsset>>& { return m_textures; }
        [[nodiscard]] auto renderState() const noexcept -> const resources::RenderState& {
            return m_renderState;
        }

    private:
        AssetHandle<RuntimeShaderAsset> m_shader{};
        std::vector<AssetHandle<RuntimeTextureAsset>> m_textures;
        resources::RenderState m_renderState{};
    };

    template<typename T>
    struct AssetTraits;

    template<> struct AssetTraits<RuntimeMeshAsset> {
        static constexpr AssetType type = AssetType::Mesh;
    };
    template<> struct AssetTraits<RuntimeTextureAsset> {
        static constexpr AssetType type = AssetType::Texture;
    };
    template<> struct AssetTraits<RuntimeShaderAsset> {
        static constexpr AssetType type = AssetType::Shader;
    };
    template<> struct AssetTraits<RuntimeMaterialAsset> {
        static constexpr AssetType type = AssetType::Material;
    };
}

#endif // AETHERENGINE_ASSETS_RUNTIMEASSET_HPP
