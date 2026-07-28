//
// Created by drhaz on 23.07.2026.
//

#include "PhysicsBackendFactory.hpp"

#include "physx/PhysXBackend.hpp"

namespace engine::physics {
    auto createPhysicsBackend() -> std::unique_ptr<IPhysicsBackend> {
        return std::make_unique<PhysXBackend>();
    }
}
