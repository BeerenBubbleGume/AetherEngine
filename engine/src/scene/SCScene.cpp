//
// Created by drhaz on 21.06.2026.
//

#include "scene/SCScene.hpp"


namespace engine::scene {
    SCScene::SScenePtr SCScene::createScene() {
        return SScenePtr(new SCScene(), SCSceneDeleter{});
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
} // scene
