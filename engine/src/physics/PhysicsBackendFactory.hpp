//
// Created by drhaz on 23.07.2026.
//

#ifndef SMB_PHYSICSBACKENDFACTORY_HPP
#define SMB_PHYSICSBACKENDFACTORY_HPP
#include <memory>

#include "IPhysicsBackend.hpp"


namespace engine::physics {
    [[nodiscard]] auto createPhysicsBackend() -> std::unique_ptr<IPhysicsBackend>;
}


#endif //SMB_PHYSICSBACKENDFACTORY_HPP
