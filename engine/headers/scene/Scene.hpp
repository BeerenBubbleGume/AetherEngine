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

namespace engine::systems {class SceneSerializer;}
namespace engine::components {struct IdentityComponent;}

namespace engine::scene {
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
        auto createEntity() -> Entity;

        template<typename T, typename... Args>
        constexpr T& addComponent(Entity entt, Args&&... args);
        template<typename T>
        constexpr T &getComponent(Entity entt);
        template <typename... T> auto view();
        template <typename... T> void destroyEntity(Entity entt);
        template <typename... T> void removeComponent(Entity entt);
        template <typename... T> [[nodiscard]] bool hasComponent(Entity entt) const;
        template <typename... T> auto tryGetComponent(Entity entt);

        auto setActiveCamera(Entity camera) -> void;
        [[nodiscard]] auto getActiveCamera() const -> Entity;
        auto findEntityById(std::string_view id) -> std::optional<Entity>;
    private:
        friend class systems::SceneSerializer;

        [[nodiscard]] auto getRegistry() const -> const entt::registry &;
        Scene(std::string_view name);
        Scene() = default;
        ~Scene() = default;
        entt::registry m_registry;
        std::optional<Entity> m_activeCamera;
        const std::string m_name;
    };

    template<typename T, typename ... Args>
    constexpr T & Scene::addComponent(Entity entt, Args &&...args) {
        if (entt.isValid()) {
            return m_registry.emplace<T>(entt.handle, std::forward<Args>(args)...);
        } else {
            throw std::runtime_error("Invalid entity");
        }
    }

    template<typename T>
    constexpr T &Scene::getComponent(const Entity entt) {
        if (!entt.isValid()) {
            throw std::runtime_error("Invalid entity");
        }
        return m_registry.get<T>(entt.handle);
    }

    template<typename... T>
    auto Scene::view() {
        return m_registry.view<T...>();
    }

    template<typename ... T>
    void Scene::destroyEntity(Entity entt) {
        m_registry.destroy(entt.handle);
    }

    template<typename ... T>
    void Scene::removeComponent(Entity entt) {
        m_registry.remove<T...>(entt.handle);
    }

    template<typename ... T>
    bool Scene::hasComponent(Entity entt) const {
        return m_registry.all_of<T...>(entt.handle);
    }

    template<typename ... T>
    auto Scene::tryGetComponent(Entity entt) {
        return m_registry.try_get<T...>(entt.handle);
    }
} // scene
// engine

#endif //SMB_SCENE_HPP
