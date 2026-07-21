//
// Created by drhaz on 24.06.2026.
//

#ifndef SMB_ENTITY_HPP
#define SMB_ENTITY_HPP

#include <entt/entity/entity.hpp>

namespace engine::scene {
    class Scene;
    struct Entity {
        entt::entity handle{entt::null};
        Scene* scene{nullptr};

        [[nodiscard]] bool isValid() const {
            return handle != entt::null && scene != nullptr;
        }
    };
}

#endif //SMB_ENTITY_HPP
