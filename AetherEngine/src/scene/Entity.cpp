//
// Created by drhaz on 27.09.2026.
//

#include "scene/Entity.hpp"

#include "scene/Scene.hpp"

namespace AetherEngine::scene {
    bool Entity::isValid() const {
        return scene != nullptr && scene->isAlive(*this);
    }
}
