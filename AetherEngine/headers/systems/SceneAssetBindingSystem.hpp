//
// Created by drhaz on 03.09.2026.
//

#ifndef SMB_SCENEASSETBINDINGSYSTEM_HPP
#define SMB_SCENEASSETBINDINGSYSTEM_HPP
#include "assets/AssetId.hpp"
#include "assets/AssetManager.hpp"
#include "scene/Scene.hpp"


namespace AetherEngine::systems {
    struct SceneAssetBindingError {
        assets::AssetId asset{};
        std::string message;
    };
    class SceneAssetBindingSystem {
    public:
        struct SceneAssetBindingSystemDeleter {
            void operator()(SceneAssetBindingSystem* system) {
                delete system;
            }
        };
        using SceneAssetBindingSystemPtr = std::unique_ptr<SceneAssetBindingSystem, SceneAssetBindingSystemDeleter>;
        [[nodiscard]] static auto create(assets::AssetManager& assets) -> SceneAssetBindingSystemPtr;
        SceneAssetBindingSystem(SceneAssetBindingSystem const&) = delete;
        SceneAssetBindingSystem& operator=(SceneAssetBindingSystem const&) = delete;
        SceneAssetBindingSystem(SceneAssetBindingSystem&&) = delete;
        SceneAssetBindingSystem& operator=(SceneAssetBindingSystem&&) = delete;

        [[nodiscard]] auto bindScene(scene::Scene& scene) -> std::expected<void, SceneAssetBindingError>;
        [[nodiscard]] auto synchronize(scene::Scene& scene) -> std::expected<void, SceneAssetBindingError>;
        auto unbindScene() noexcept -> void;

        [[nodiscard]] auto meshHandle(assets::AssetRef<assets::RuntimeMeshAsset> ref) const -> std::optional<assets::AssetHandle<assets::RuntimeMeshAsset>>;
        [[nodiscard]] auto materialHandle(assets::AssetRef<assets::RuntimeMaterialAsset> ref) const -> std::optional<assets::AssetHandle<assets::RuntimeMaterialAsset>>;

    private:
        assets::AssetManager* m_assets{nullptr};
        scene::Scene* m_scene{nullptr};

        template <typename T>
        struct SceneAssetBinding {
            assets::AssetRef<T> reference;
            assets::AssetHandle<T> runtime;
            std::size_t users{};
        };

        using MeshBinding = SceneAssetBinding<assets::RuntimeMeshAsset>;
        using MaterialBinding = SceneAssetBinding<assets::RuntimeMaterialAsset>;

        std::unordered_map<assets::AssetId, MeshBinding, assets::AssetIdHash> m_meshes;
        std::unordered_map<assets::AssetId, MaterialBinding, assets::AssetIdHash> m_materials;

        explicit SceneAssetBindingSystem(assets::AssetManager& assets);
        ~SceneAssetBindingSystem();
    };
} // systems
// AetherEngine

#endif //SMB_SCENEASSETBINDINGSYSTEM_HPP
