//
// Created by drhaz on 21.06.2026.
//

#ifndef SMB_SCENE_HPP
#define SMB_SCENE_HPP

#include <vector>
#include <array>
#include <span>
#include <entt/entt.hpp>

#include "graphics/Mesh.hpp"
#include "graphics/ShaderProgram.hpp"
#include "resources/ResourceManager.hpp"
#include "Entity.hpp"
#include "EntityId.hpp"
#include "components/CameraComponent.hpp"
#include "components/IdentityComponent.hpp"

namespace AetherEngine::systems {class SceneSerializer;}
namespace AetherEngine::components {struct IdentityComponent;}

namespace AetherEngine::scene {
    class Scene {
    public:
        struct SceneDeleter {
            void operator()(Scene* scene) const {
                delete scene;
            }
        };
        using ScenePtr = std::unique_ptr<Scene, SceneDeleter>;
        Scene(const Scene&) = delete;
        Scene& operator=(const Scene&) = delete;


        static ScenePtr createScene();
        static ScenePtr createScene(std::string_view name);

        [[nodiscard]] auto getName() const -> std::string_view;
        [[nodiscard]] auto isEmpty() const -> bool;
        auto createEntity(std::string_view name) -> Entity;
        auto createEntityWithId(scene::EntityId id, std::string_view name) -> Entity;

        template<typename T, typename... Args>
        constexpr T& addComponent(Entity entt, Args&&... args);
        template<typename T>
        constexpr T &getComponent(Entity entt);
        template <typename... T> auto view();
        template <typename... T> void removeComponent(Entity entt);
        template <typename... T> [[nodiscard]] bool hasComponent(Entity entt) const;
        template <typename... T> auto tryGetComponent(Entity entt);
        auto destroyEntity(Entity entt) -> void;

        auto setActiveCamera(Entity camera) -> void;
        [[nodiscard]] auto getActiveCamera() const -> Entity;
        auto findEntityByName(std::string_view name) -> std::optional<Entity>;
        auto findEntityById(scene::EntityId id) -> std::optional<Entity>;

        [[nodiscard]] [[nodiscard]] auto isAlive(const Entity &entity) const -> bool;
    private:
        friend class systems::SceneSerializer;

        [[nodiscard]] auto getRegistry() const -> const entt::registry &;
        Scene(std::string_view name);
        Scene() = default;
        ~Scene() = default;
        entt::registry m_registry;
        std::optional<Entity> m_activeCamera;
        std::unordered_map<EntityId, entt::entity, EntityIdHash> m_entitiesById;
        const std::string m_name;
    };

    template<typename T, typename ... Args>
    constexpr T & Scene::addComponent(Entity entt, Args &&...args) {
        if (this->isAlive(entt)) {
            return m_registry.emplace<T>(entt.handle, std::forward<Args>(args)...);
        } else {
            throw std::runtime_error("Invalid entity");
        }
    }

    template<typename T>
    constexpr T &Scene::getComponent(const Entity entt) {
        if (!this->isAlive(entt)) {
            throw std::runtime_error("Invalid entity");
        }
        return m_registry.get<T>(entt.handle);
    }

    template<typename... T>
    auto Scene::view() {
        return m_registry.view<T...>();
    }

    template<typename ... T>
    void Scene::removeComponent(Entity entt) {
        if (!this->isAlive(entt)) {
            throw std::runtime_error("Invalid entity");
        } if constexpr ((std::is_same_v<T, components::IdentityComponent> || ...)) {
            throw std::runtime_error("Invalid entity");
        }
        m_registry.remove<T...>(entt.handle);
        if constexpr ((std::is_same_v<T, components::CameraComponent> || ...)) {
            if (m_activeCamera && m_activeCamera->scene == entt.scene &&
                m_activeCamera->handle == entt.handle) {
                m_activeCamera.reset();
                }
        }
    }

    template<typename ... T>
    bool Scene::hasComponent(Entity entt) const {
        if (!this->isAlive(entt)) {
            throw std::runtime_error("Invalid entity");
        }
        return m_registry.all_of<T...>(entt.handle);
    }

    template<typename ... T>
    auto Scene::tryGetComponent(Entity entt) {
        if (!this->isAlive(entt)) {
            throw std::runtime_error("Invalid entity");
        }
        return m_registry.try_get<T...>(entt.handle);
    }
} // scene
// AetherEngine

#endif //SMB_SCENE_HPP
