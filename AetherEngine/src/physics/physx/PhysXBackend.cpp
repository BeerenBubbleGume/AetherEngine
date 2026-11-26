//
// Created by drhaz on 23.07.2026.
//

#include "PhysXBackend.hpp"

#include <cmath>
#include "physics/PhysicsValidation.hpp"

namespace AetherEngine::physics {
    namespace {
        [[nodiscard]] auto toPxVec3(math::Vec3 value) -> physx::PxVec3 {
            return {value.x, value.y, value.z};
        }

        [[nodiscard]] auto toPxQuat(math::Quat value) -> physx::PxQuat {
            return {value.x, value.y, value.z, value.w};
        }

        [[nodiscard]] auto fromPxVec3(const physx::PxVec3& value) -> math::Vec3 {
            return {value.x, value.y, value.z};
        }

        [[nodiscard]] auto fromPxQuat(const physx::PxQuat& value) -> math::Quat {
            return {value.w, value.x, value.y, value.z};
        }

        [[nodiscard]] auto toValidPxTransform(const math::Transform& transform)
            -> std::optional<physx::PxTransform> {
            const auto position = toPxVec3(transform.position);
            const auto rotation = toPxQuat(transform.rotation);
            if (!position.isFinite() || !rotation.isFinite() || !rotation.isUnit()) {
                return std::nullopt;
            }

            const physx::PxTransform result{position, rotation};
            return result.isValid()
                ? std::optional<physx::PxTransform>{result}
                : std::nullopt;
        }
    }

    PhysXBackend::~PhysXBackend() {
        shutdown();
    }

    auto PhysXBackend::init() -> std::expected<void, PhysicsError> {
        if (m_scene) {
            return {};
        }

        const physx::PxTolerancesScale scale{1.0f, 10.0f};

        m_foundation = PxCreateFoundation(PX_PHYSICS_VERSION, m_allocator, m_errorCallback);
        if (!m_foundation) {
            return std::unexpected(PhysicsError{1, "Failed to create PhysX foundation"});
        }

        m_physics = PxCreatePhysics(PX_PHYSICS_VERSION, *m_foundation, scale, true);
        if (!m_physics) {
            shutdown();
            return std::unexpected(PhysicsError{1, "Failed to create PhysX physics"});
        }

        m_dispatcher = physx::PxDefaultCpuDispatcherCreate(1);
        if (!m_dispatcher) {
            shutdown();
            return std::unexpected(PhysicsError{1, "Failed to create PhysX dispatcher"});
        }

        physx::PxSceneDesc sceneDesc{scale};
        sceneDesc.gravity = physx::PxVec3{0.0f, -9.81f, 0.0f};
        sceneDesc.cpuDispatcher = m_dispatcher;
        sceneDesc.filterShader = physx::PxDefaultSimulationFilterShader;

        if (!sceneDesc.isValid()) {
            shutdown();
            return std::unexpected(PhysicsError{1, "Invalid PhysX scene descriptor"});
        }

        m_scene = m_physics->createScene(sceneDesc);
        if (!m_scene) {
            shutdown();
            return std::unexpected(PhysicsError{1, "Failed to create PhysX scene"});
        }

        m_defaultMaterial = m_physics->createMaterial(0.5f, 0.5f, 0.5f);
        if (!m_defaultMaterial) {
            shutdown();
            return std::unexpected(PhysicsError{1, "Failed to create PhysX material"});
        }

        return {};
    }

    auto PhysXBackend::shutdown() -> void {
        for (const auto& [handle, body] : m_bodies) {
            static_cast<void>(handle);
            if (!body) {
                continue;
            }
            if (m_scene) {
                m_scene->removeActor(*body);
            }
            body->release();
        }
        m_bodies.clear();

        if (m_defaultMaterial) m_defaultMaterial->release();
        if (m_scene) m_scene->release();
        if (m_physics) m_physics->release();
        if (m_dispatcher) m_dispatcher->release();
        if (m_foundation) m_foundation->release();

        m_defaultMaterial = nullptr;
        m_scene = nullptr;
        m_physics = nullptr;
        m_dispatcher = nullptr;
        m_foundation = nullptr;
        m_nextBodyHandle = 1;
    }

    auto PhysXBackend::setTransform(
        BodyHandle handle,
        const math::Transform& transform
    ) -> void {
        const auto bodyIt = m_bodies.find(handle.value);
        if (bodyIt == m_bodies.end()) {
            return;
        }

        if (const auto pxTransform = toValidPxTransform(transform)) {
            bodyIt->second->setGlobalPose(*pxTransform);
        }
    }

    auto PhysXBackend::simulate(float fixedDelta) -> void {
        if (!m_scene || !std::isfinite(fixedDelta) || fixedDelta <= 0.0f) {
            return;
        }

        m_scene->simulate(fixedDelta);
        m_scene->fetchResults(true);
    }

    auto PhysXBackend::createBody(const BodyDesc& desc) -> std::expected<BodyHandle, PhysicsError> {
        if (!m_physics || !m_scene || !m_defaultMaterial) {
            return std::unexpected(PhysicsError{1, "PhysX is not initialized"});
        }

        const auto pxTransform = toValidPxTransform(desc.transform);
        if (!pxTransform) return std::unexpected(PhysicsError{2, "Transform position or rotation is invalid"});
        if (const auto field = invalidPhysicsField(desc.collider); !field.empty())
            return std::unexpected(PhysicsError{2, "Invalid " + std::string{field}});
        const auto pxCollider = createGeometry(desc.collider);
        if (!pxCollider) {
            return std::unexpected(PhysicsError{2, "PhysX rejected collider geometry"});
        }

        if (!validPositivePhysicsValue(desc.mass, MaxBodyMass)) {
            return std::unexpected(PhysicsError{2, "Invalid Rigidbody.mass"});
        }

        physx::PxRigidActor* body = desc.dynamic
            ? static_cast<physx::PxRigidActor*>(
                m_physics->createRigidDynamic(*pxTransform)
            )
            : static_cast<physx::PxRigidActor*>(
                m_physics->createRigidStatic(*pxTransform)
            );
        if (!body) {
            return std::unexpected(PhysicsError{3, "PhysX could not allocate an actor"});
        }

        physx::PxShape* shape = m_physics->createShape(
            pxCollider->any(),
            *m_defaultMaterial
        );
        if (!shape) {
            body->release();
            return std::unexpected(PhysicsError{3, "PhysX could not allocate a shape"});
        }

        const auto& collider = desc.collider;
        shape->setFlag(physx::PxShapeFlag::eSIMULATION_SHAPE,
               collider.collisionEnabled && !collider.trigger);
        shape->setFlag(physx::PxShapeFlag::eTRIGGER_SHAPE,
                       collider.collisionEnabled && collider.trigger);

        if (!body->attachShape(*shape)) {
            shape->release();
            body->release();
            return std::unexpected(PhysicsError{3, "PhysX could not attach the collider"});
        }
        shape->release();

        if (desc.dynamic) {
            auto* dynamicBody = static_cast<physx::PxRigidDynamic*>(body);
            dynamicBody->setActorFlag(
                physx::PxActorFlag::eDISABLE_GRAVITY,
                !desc.useGravity
            );
            // Sensors still have geometry and need the same mass properties.
            if (!physx::PxRigidBodyExt::setMassAndUpdateInertia(*dynamicBody, desc.mass, nullptr, true)) {
                body->release();
                return std::unexpected(PhysicsError{2, "PhysX could not calculate mass and inertia for this collider"});
            }
        }

        const auto handle = allocateBodyHandle();
        if (!handle.isValid()) {
            body->release();
            return std::unexpected(PhysicsError{3, "PhysX body handle limit reached"});
        }

        try {
            m_bodies.emplace(handle.value, body);
        } catch (...) {
            body->release();
            throw;
        }
        m_scene->addActor(*body);
        return handle;
    }

    auto PhysXBackend::destroyBody(BodyHandle handle) noexcept -> void {
        const auto bodyIt = m_bodies.find(handle.value);
        if (bodyIt == m_bodies.end()) {
            return;
        }

        if (m_scene) {
            m_scene->removeActor(*bodyIt->second);
        }
        bodyIt->second->release();
        m_bodies.erase(bodyIt);
    }

    auto PhysXBackend::getTransform(BodyHandle handle) const -> math::Transform {
        const auto bodyIt = m_bodies.find(handle.value);
        if (bodyIt == m_bodies.end()) {
            return {};
        }

        const auto pxTransform = bodyIt->second->getGlobalPose();
        math::Transform transform;
        transform.position = fromPxVec3(pxTransform.p);
        transform.rotation = fromPxQuat(pxTransform.q);
        return transform;
    }

    auto PhysXBackend::createGeometry(const components::ColliderComponent& collider)
        -> std::optional<physx::PxGeometryHolder> {
        constexpr float MaxColliderDimension = 100000.0f;
        const auto validDimension = [](float value) {
            return std::isfinite(value) &&
                value > 0.0f &&
                value <= MaxColliderDimension;
        };

        switch (collider.type) {
            case components::ColliderType::Box: {
                if (!validDimension(collider.size.x) ||
                    !validDimension(collider.size.y) ||
                    !validDimension(collider.size.z)) {
                    return std::nullopt;
                }

                const physx::PxBoxGeometry geometry{
                    collider.size.x,
                    collider.size.y,
                    collider.size.z
                };
                return geometry.isValid()
                    ? std::optional<physx::PxGeometryHolder>{geometry}
                    : std::nullopt;
            }

            case components::ColliderType::Sphere: {
                if (!validDimension(collider.radius)) {
                    return std::nullopt;
                }

                const physx::PxSphereGeometry geometry{collider.radius};
                return geometry.isValid()
                    ? std::optional<physx::PxGeometryHolder>{geometry}
                    : std::nullopt;
            }

            case components::ColliderType::Capsule: {
                if (!validDimension(collider.radius) ||
                    !validDimension(collider.height)) {
                    return std::nullopt;
                }

                const physx::PxCapsuleGeometry geometry{
                    collider.radius,
                    collider.height * 0.5f
                };
                return geometry.isValid()
                    ? std::optional<physx::PxGeometryHolder>{geometry}
                    : std::nullopt;
            }
        }

        return std::nullopt;
    }

    auto PhysXBackend::allocateBodyHandle() -> BodyHandle {
        if (m_nextBodyHandle == 0) {
            m_nextBodyHandle = 1;
        }

        while (m_bodies.contains(m_nextBodyHandle)) {
            ++m_nextBodyHandle;
            if (m_nextBodyHandle == 0) {
                m_nextBodyHandle = 1;
            }
        }

        return BodyHandle{m_nextBodyHandle++};
    }
} // physics
// AetherEngine
