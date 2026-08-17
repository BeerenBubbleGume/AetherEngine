//
// Created by drhaz on 21.06.2026.
//

#include "scene/Scene.hpp"

#include "components/IdentityComponent.hpp"


namespace AetherEngine::scene {
    Scene::ScenePtr Scene::createScene() {
        return ScenePtr(new Scene(), SceneDeleter{});
    }

    Scene::ScenePtr Scene::createScene(std::string_view name) {
        return ScenePtr(new Scene(name), SceneDeleter{});
    }

    auto Scene::getName() const -> std::string_view {
        return m_name;
    }

    auto Scene::isEmpty() const -> bool {
        const auto* entityStorage = m_registry.storage<entt::entity>();
        return !entityStorage || entityStorage->empty();
    }

    auto Scene::createEntity() -> Entity {
        return Entity({m_registry.create(), this});
    }

    auto Scene::setActiveCamera(Entity camera) -> void {
        m_activeCamera = camera;
    }

    auto Scene::getActiveCamera() const -> Entity {
        if (m_activeCamera.has_value()) {
            return m_activeCamera.value();
        } else {
            return Entity{entt::null, nullptr};
        }
    }

    auto Scene::findEntityById(std::string_view id) -> std::optional<Entity> {
        auto view = m_registry.view<AetherEngine::components::IdentityComponent>();

        for (auto entity : view) {
            const auto& identity = view.get<components::IdentityComponent>(entity);
            if (identity.id == id) {
                return Entity{entity, this};
            }
        }

        return std::nullopt;
    }

    auto Scene::getRegistry() const -> const entt::registry & {
        return m_registry;
    }

    Scene::Scene(std::string_view name) : m_name(name) {
    }
} // scene
