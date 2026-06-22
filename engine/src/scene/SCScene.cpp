//
// Created by drhaz on 21.06.2026.
//

#include "scene/SCScene.hpp"


namespace engine::scene {
    SCScene::SScenePtr SCScene::createScene() {
        return SScenePtr(new SCScene(), SCSceneDeleter{});
    }

    auto SCScene::addObject(SRenderObject object) -> SObjectId {
        m_objects.push_back(std::move(object));
        return m_objects.size() - 1;
    }

    auto SCScene::addCamera(SCCamera camera) -> void {
        m_camera = std::make_unique<SCCamera>(std::move(camera));
    }

    auto SCScene::getCamera() -> SCCamera & {
        if (!m_camera) {
            throw std::runtime_error("No camera added to scene");
        }
        return *m_camera;
    }

    auto SCScene::renderObjects() const -> std::span<const SRenderObject> {
        return m_objects;
    }

    auto SCScene::getObject(SObjectId id) -> SRenderObject& {
        return m_objects.at(id);
    }
} // scene
