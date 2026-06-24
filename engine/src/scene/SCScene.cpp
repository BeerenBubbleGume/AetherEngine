//
// Created by drhaz on 21.06.2026.
//

#include "scene/SCScene.hpp"


namespace engine::scene {
    SCScene::SScenePtr SCScene::createScene() {
        return SScenePtr(new SCScene(), SCSceneDeleter{});
    }

    auto SCScene::addCamera(SCCamera camera) -> void {
        m_camera = std::make_unique<SCCamera>(std::move(camera));
    }

    auto SCScene::createEntity() -> Entity {
        return Entity({m_registry.create(), this});
    }

    auto SCScene::getCamera() -> SCCamera & {
        if (!m_camera) {
            throw std::runtime_error("No camera added to scene");
        }
        return *m_camera;
    }
} // scene
