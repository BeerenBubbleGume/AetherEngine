//
// Created by drhaz on 23.07.2026.
//

#include "PhysicsBackendFactory.hpp"

#if defined(SMB_PHYSICS_BACKEND_JOLT)
#include "jolt/JoltBackend.hpp"
#elif defined(SMB_PHYSICS_BACKEND_PHYSX)
#include "physx/PhysXBackend.hpp"
#else
#error "A physics backend must be selected by the build system"
#endif

namespace AetherEngine::physics {
    auto createPhysicsBackend() -> std::unique_ptr<IPhysicsBackend> {
#if defined(SMB_PHYSICS_BACKEND_JOLT)
        return std::make_unique<JoltBackend>();
#else
        return std::make_unique<PhysXBackend>();
#endif
    }
}
