//
// Created by drhaz on 03.09.2026.
//

#include "systems/SceneAssetBindingSystem.hpp"

#include "components/MaterialComponent.hpp"
#include "components/MeshComponent.hpp"


namespace AetherEngine::systems {
    auto SceneAssetBindingSystem::create(assets::AssetManager &assets) -> SceneAssetBindingSystemPtr {
        return SceneAssetBindingSystemPtr(new SceneAssetBindingSystem(assets), SceneAssetBindingSystemDeleter{});
    }

    auto SceneAssetBindingSystem::bindScene(scene::Scene &scene) -> std::expected<void, SceneAssetBindingError> {
        std::unordered_map<assets::AssetId,std::size_t, assets::AssetIdHash> requiredMeshes;
        std::unordered_map<assets::AssetId,std::size_t, assets::AssetIdHash> requiredMaterials;
        auto meshes = scene.view<components::MeshComponent>();
        auto materials = scene.view<components::MaterialComponent>();

        for (auto entity : meshes) {
            const auto& component = meshes.get<components::MeshComponent>(entity);
            const auto id = component.asset.id;
            auto& count = requiredMeshes[id];
            ++count;
        }
        for (auto entity : materials) {
            const auto& component = materials.get<components::MaterialComponent>(entity);

            const auto id = component.asset.id;
            auto& count = requiredMaterials[id];
            ++count;
        }

        std::unordered_map<assets::AssetId, MeshBinding, assets::AssetIdHash> newBindings;
        std::unordered_map<assets::AssetId, MaterialBinding, assets::AssetIdHash> materialBindings;

        struct BindingCleanup {
            assets::AssetManager& assets;

            decltype(newBindings)& meshes;
            decltype(materialBindings)& materials;

            ~BindingCleanup() {
                for (const auto& [id, binding] : meshes) {
                    if (binding.runtime.isValid()) {
                        assets.unload(id);
                    }
                }

                for (const auto& [id, binding] : materials) {
                    if (binding.runtime.isValid()) {
                        assets.unload(id);
                    }
                }
            }
        };

        BindingCleanup cleanup{
            *m_assets,
            newBindings,
            materialBindings
        };

        for (const auto& [id, users] : requiredMeshes) {
            assets::AssetRef<assets::RuntimeMeshAsset> ref{id};
            auto position = newBindings.emplace(id, MeshBinding{
                .reference = ref,
                .runtime = {},
                .users = users
            }).first;

            auto loaded = m_assets->load(ref);

            if (!loaded) {
                return std::unexpected(SceneAssetBindingError{
                    id,
                    "Failed to load mesh asset: " + loaded.error().message
                });
            }
            position->second.runtime = *loaded;
        }

        for (const auto& [id, users] : requiredMaterials) {
            assets::AssetRef<assets::RuntimeMaterialAsset> ref{id};

            auto position = materialBindings.emplace(id, MaterialBinding{
                .reference = ref,
                .runtime = {},
                .users = users
            }).first;

            auto loaded = m_assets->load(ref);

            if (!loaded) {
                return std::unexpected(SceneAssetBindingError{
                    id,
                    "Failed to load material asset: " + loaded.error().message
                });
            }

            position->second.runtime = *loaded;
        }
        m_meshes.swap(newBindings);
        m_materials.swap(materialBindings);

        m_scene = &scene;

        return {};
    }

    auto SceneAssetBindingSystem::synchronize(scene::Scene &scene) -> std::expected<void, SceneAssetBindingError> {
        if (m_scene != &scene) {
            return std::unexpected(SceneAssetBindingError{
                {},
                "Cannot synchronize a scene that is not bound"
            });
        }
        /*TODO: implement new synchronization logic*/
        return bindScene(scene);
    }

    auto SceneAssetBindingSystem::unbindScene() noexcept -> void {
        for (const auto& [id, binding] : m_meshes) {
            if (binding.runtime.isValid()) {
                m_assets->unload(id);
            }
        }
        for (const auto& [id, binding] : m_materials) {
            if (binding.runtime.isValid()) {
                m_assets->unload(id);
            }
        }
        m_meshes.clear();
        m_materials.clear();
        m_scene = nullptr;
    }

    auto SceneAssetBindingSystem::meshHandle(
        assets::AssetRef<assets::RuntimeMeshAsset> ref) const -> std::optional<assets::AssetHandle<assets::
        RuntimeMeshAsset>> {
        auto it = m_meshes.find(ref.id);
        if (it == std::end(m_meshes)) {
            return std::nullopt;
        }
        return it->second.runtime;
    }

    auto SceneAssetBindingSystem::materialHandle(
        assets::AssetRef<assets::RuntimeMaterialAsset> ref) const -> std::optional<assets::AssetHandle<assets::
        RuntimeMaterialAsset>> {
        auto it = m_materials.find(ref.id);
        if (it == std::end(m_materials)) {
            return std::nullopt;
        }
        return it->second.runtime;
    }

    SceneAssetBindingSystem::SceneAssetBindingSystem(assets::AssetManager &assets) : m_assets(&assets) {
    }

    SceneAssetBindingSystem::~SceneAssetBindingSystem() {
        unbindScene();
    }
} // systems
// AetherEngine