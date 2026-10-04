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
        const auto* storage = m_registry.storage<entt::entity>();
        return !storage || storage->free_list() == 0;
    }

    auto Scene::createEntity(std::string_view name) -> Entity {
        EntityId id;

        do {
            id = generateEntityId();
        } while (!id.isValid() || findEntityById(id).has_value());

        return createEntityWithId(id, name);
    }

    auto Scene::createEntityWithId(scene::EntityId id, std::string_view name) -> Entity {
        if (!id.isValid()) {
            throw std::runtime_error("Invalid EntityId");
        }
        if (m_entitiesById.contains(id)) {
            throw std::runtime_error("Duplicate EntityId");
        }
        auto entity = m_registry.create();
        m_registry.emplace<components::IdentityComponent>(entity, components::IdentityComponent{
            .id = id,
            .name = std::string{name}
        });
        m_entitiesById.emplace(id, entity);
        return Entity({.handle = entity, .scene = this});
    }

    auto Scene::setActiveCamera(Entity camera) -> void {
        if (!this->isAlive(camera)) {
            throw std::runtime_error("Invalid entity");
        }
        if (!m_registry.all_of<components::CameraComponent>(camera.handle)) {
            throw std::runtime_error("Entity is not a camera");
        }
        m_activeCamera = camera;
    }

    auto Scene::getActiveCamera() const -> Entity {
        if (m_activeCamera.has_value()) {
            return m_activeCamera.value();
        } else {
            return Entity{entt::null, nullptr};
        }
    }

    auto Scene::findEntityByName(std::string_view name) -> std::optional<Entity> {
        auto view = m_registry.view<AetherEngine::components::IdentityComponent>();

        for (auto entity : view) {
            const auto& identity = view.get<components::IdentityComponent>(entity);
            if (identity.name == name) {
                return Entity{.handle = entity, .scene = this};
            }
        }

        return std::nullopt;
    }

    auto Scene::findEntityById(scene::EntityId id) -> std::optional<Entity> {
        const auto it = m_entitiesById.find(id);

        if (it == m_entitiesById.end()) {
            return std::nullopt;
        }

        return Entity{
            .handle = it->second,
            .scene = this
        };
    }

    auto Scene::destroyEntity(Entity entt) -> void {
        if (!this->isAlive(entt)) {
            throw std::runtime_error("Invalid entity");
        }
        if (m_activeCamera &&
            m_activeCamera->scene == entt.scene &&
            m_activeCamera->handle == entt.handle) {
            m_activeCamera.reset();
            }
        const auto& identity = m_registry.get<components::IdentityComponent>(entt.handle);
        m_entitiesById.erase(identity.id);
        m_registry.destroy(entt.handle);

    }

    auto Scene::isAlive(const Entity &entity) const -> bool {
        return entity.scene == this && entity.handle != entt::null && m_registry.valid(entity.handle);
    }

    auto Scene::getRegistry() const -> const entt::registry & {
        return m_registry;
    }

    Scene::Scene(std::string_view name) : m_name(name) {
    }
} // scene
