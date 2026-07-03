//
// Created by drhaz on 21.06.2026.
//

#include "scene/SCScene.hpp"

#include "components/IIdentityComponent.hpp"


namespace engine::scene {
    SCScene::SScenePtr SCScene::createScene() {
        return SScenePtr(new SCScene(), SCSceneDeleter{});
    }

    SCScene::SScenePtr SCScene::createScene(std::string_view name) {
        return SScenePtr(new SCScene(name), SCSceneDeleter{});
    }

    auto SCScene::getName() const -> std::string_view {
        return m_name;
    }

    auto SCScene::isEmpty() const -> bool {
        const auto* entityStorage = m_registry.storage<entt::entity>();
        return !entityStorage || entityStorage->empty();
    }

    auto SCScene::createEntity() -> Entity {
        return Entity({m_registry.create(), this});
    }

    auto SCScene::setActiveCamera(Entity camera) -> void {
        m_activeCamera = camera;
    }

    auto SCScene::getActiveCamera() const -> Entity {
        if (m_activeCamera.has_value()) {
            return m_activeCamera.value();
        } else {
            return Entity{entt::null, nullptr};
        }
    }

    auto SCScene::findEntityById(std::string_view id) -> std::optional<Entity> {
        auto view = m_registry.view<engine::components::IIdentityComponent>();

        for (auto entity : view) {
            const auto& identity = view.get<components::IIdentityComponent>(entity);
            if (identity.id == id) {
                return Entity{entity, this};
            }
        }

        return std::nullopt;
    }

    auto SCScene::getRegistry() const -> const entt::registry & {
        return m_registry;
    }

    SCScene::SCScene(std::string_view name) : m_name(name) {
    }
} // scene
