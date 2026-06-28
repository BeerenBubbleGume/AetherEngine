//
// Created by drhaz on 21.06.2026.
//

#ifndef SMB_SSCENE_HPP
#define SMB_SSCENE_HPP

#include <vector>
#include <array>
#include <span>
#include <entt/entt.hpp>

#include "graphics/GMesh.hpp"
#include "graphics/GProgram.hpp"
#include "resources/RResourceManager.hpp"
#include "Entity.hpp"

namespace engine::scene {
    class SCScene {
    public:
        struct SCSceneDeleter {
            void operator()(SCScene* scene) const {
                delete scene;
            }
        };
        using SScenePtr = std::unique_ptr<SCScene, SCSceneDeleter>;
        SCScene(const SCScene&) = delete;
        SCScene& operator=(const SCScene&) = delete;


        static SScenePtr createScene();
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
    private:
        SCScene() = default;
        ~SCScene() = default;
        entt::registry m_registry;
        std::optional<Entity> m_activeCamera;
    };

    template<typename T, typename ... Args>
    constexpr T & SCScene::addComponent(Entity entt, Args &&...args) {
        if (entt.isValid()) {
            return m_registry.emplace<T>(entt.handle, std::forward<Args>(args)...);
        } else {
            throw std::runtime_error("Invalid entity");
        }
    }

    template<typename T>
    constexpr T &SCScene::getComponent(const Entity entt) {
        if (!entt.isValid()) {
            throw std::runtime_error("Invalid entity");
        }
        return m_registry.get<T>(entt.handle);
    }

    template<typename... T>
    auto SCScene::view() {
        return m_registry.view<T...>();
    }

    template<typename ... T>
    void SCScene::destroyEntity(Entity entt) {
        m_registry.destroy(entt.handle);
    }

    template<typename ... T>
    void SCScene::removeComponent(Entity entt) {
        m_registry.remove<T...>(entt.handle);
    }

    template<typename ... T>
    bool SCScene::hasComponent(Entity entt) const {
        return m_registry.all_of<T...>(entt.handle);
    }

    template<typename ... T>
    auto SCScene::tryGetComponent(Entity entt) {
        return m_registry.try_get<T...>(entt.handle);
    }
} // scene
// engine

#endif //SMB_SSCENE_HPP
