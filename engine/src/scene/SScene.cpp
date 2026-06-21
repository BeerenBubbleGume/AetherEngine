//
// Created by drhaz on 21.06.2026.
//

#include "scene/SScene.hpp"


namespace engine::scene {
    SScene::SScenePtr SScene::createScene() {
        return SScenePtr(new SScene(), SSceneDeleter{});
    }

    auto SScene::addObject(SRenderObject object) -> void {
        m_objects.push_back(std::move(object));
    }

    auto SScene::renderObjects() const -> std::span<const SRenderObject> {
        return m_objects;
    }
} // scene
