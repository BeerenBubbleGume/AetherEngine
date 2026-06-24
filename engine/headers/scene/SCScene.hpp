//
// Created by drhaz on 21.06.2026.
//

#ifndef SMB_SSCENE_HPP
#define SMB_SSCENE_HPP

#include <vector>
#include <array>
#include <span>
#include <entt/entt.hpp>

#include "SCCamera.hpp"
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
        auto addCamera(SCCamera camera) -> void;
        auto createEntity() -> Entity;
        [[nodiscard]] auto getCamera() -> SCCamera &;

        template<typename T, typename... Args>
        constexpr T& addComponent(Entity entt, Args&&... args);
        template<typename T>T& getComponent(Entity entt);
        template <typename... T> auto view();
    private:
        SCScene() = default;
        ~SCScene() = default;
        std::unique_ptr<SCCamera> m_camera;
        entt::registry m_registry;

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
    T & SCScene::getComponent(Entity entt) {
        if (!entt.isValid()) {
            throw std::runtime_error("Invalid entity");
        }
        return m_registry.get<T>(entt.handle);
    }

    template<typename... T>
    auto SCScene::view() {
        return m_registry.view<T...>();
    }
} // scene
// engine

#endif //SMB_SSCENE_HPP
