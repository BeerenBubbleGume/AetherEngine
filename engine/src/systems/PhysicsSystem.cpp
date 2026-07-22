//
// Created by drhaz on 03.07.2026.
//

#include "systems/PhysicsSystem.hpp"

#include <cmath>
#include <optional>

namespace engine::systems {
    constexpr std::size_t MaxPhysicsActors = 512;

    static auto toPxVec3(engine::math::Vec3 v) -> physx::PxVec3 {
        return {v.x, v.y, v.z};
    }

    static auto toPxQuat(engine::math::Quat q) -> physx::PxQuat {
        return {q.x, q.y, q.z, q.w};
    }

    static auto fromPxVec3(const physx::PxVec3& v) -> engine::math::Vec3 {
        return {v.x, v.y, v.z};
    }

    static auto fromPxQuat(const physx::PxQuat& q) -> engine::math::Quat {
        return {q.w, q.x, q.y, q.z};
    }

    static auto toValidPxTransform(const components::TransformComponent& transform)
        -> std::optional<physx::PxTransform> {
        const auto position = toPxVec3(transform.transform.position);
        const auto rotation = toPxQuat(transform.transform.rotation);
        if (!position.isFinite() || !rotation.isFinite() || !rotation.isUnit()) {
            return std::nullopt;
        }
        const physx::PxTransform result{position, rotation};
        return result.isValid() ? std::optional<physx::PxTransform>{result} : std::nullopt;
    }

    PhysicsSystem::PhysicsSystemPtr PhysicsSystem::createPhysicsSystem() {
        return PhysicsSystemPtr(new PhysicsSystem(), PhysicsSystemDeleter{});
    }

    auto PhysicsSystem::PhysicsSystemDeleter::operator()(PhysicsSystem* system) const -> void {
        delete system;
    }

    auto PhysicsSystem::init() -> std::expected<void, PhysicsError> {
        const physx::PxTolerancesScale scale{1.0f, 10.0f};

        m_foundation = PxCreateFoundation(PX_PHYSICS_VERSION, m_allocator, m_errorCallback);
        if (!m_foundation) {
            return std::unexpected(PhysicsError{1, "Failed to create PhysX foundation"});
        }

        m_physics = PxCreatePhysics(PX_PHYSICS_VERSION, *m_foundation, scale, true);
        if (!m_physics) {
            return std::unexpected(PhysicsError{1, "Failed to create PhysX physics"});
        }

        m_dispatcher = physx::PxDefaultCpuDispatcherCreate(1);
        if (!m_dispatcher) {
            return std::unexpected(PhysicsError{1, "Failed to create PhysX dispatcher"});
        }

        physx::PxSceneDesc sceneDesc{scale};
        sceneDesc.gravity = physx::PxVec3{0.0f, -9.81f, 0.0f};
        sceneDesc.cpuDispatcher = m_dispatcher;
        sceneDesc.filterShader = physx::PxDefaultSimulationFilterShader;

        if (!sceneDesc.isValid()) {
            return std::unexpected(PhysicsError{1, "Invalid PhysX scene descriptor"});
        }

        m_scene = m_physics->createScene(sceneDesc);
        if (!m_scene) {
            return std::unexpected(PhysicsError{1, "Failed to create PhysX scene"});
        }

        m_defaultMaterial = m_physics->createMaterial(0.5f, 0.5f, 0.5f);
        if (!m_defaultMaterial) {
            return std::unexpected(PhysicsError{1, "Failed to create PhysX material"});
        }

        return {};
    }

    auto PhysicsSystem::shutdown() -> void {
        for (auto& [entity, actor] : m_actors) {
            static_cast<void>(entity);
            if (!actor) {
                continue;
            }
            if (m_scene) {
                m_scene->removeActor(*actor);
            }
            actor->release();
        }
        m_actors.clear();
        if (m_defaultMaterial) m_defaultMaterial->release();
        if (m_scene) m_scene->release();
        if (m_physics) m_physics->release();
        if (m_dispatcher) m_dispatcher->release();
        if (m_foundation) m_foundation->release();
        m_scene = nullptr;
        m_physics = nullptr;
        m_foundation = nullptr;
        m_dispatcher = nullptr;
        m_defaultMaterial = nullptr;
    }

    auto PhysicsSystem::fixedUpdate(scene::Scene &scene, float fixedDelta) -> void {
        destroyRemovedActors(scene);
        createMissingActors(scene);
        syncActorsFromScene(scene);
        /*applyPendingForcesAndImpulses();*/

        simulate(fixedDelta);

        syncSceneFromActors(scene);
    }

    auto PhysicsSystem::syncActorsFromScene(scene::Scene &scene) -> void {
        auto view = scene.view<components::TransformComponent, components::RigidbodyComponent, components::ColliderComponent>();
        for (auto handle : view) {
            auto actorIt = m_actors.find(handle);
            if (actorIt == m_actors.end()) {
                continue;
            }
            const auto& rigidbody = view.get<components::RigidbodyComponent>(handle);
            if (rigidbody.dynamic) {
                continue;
            }
            const auto& transform = view.get<components::TransformComponent>(handle);
            if (const auto pxTransform = toValidPxTransform(transform)) {
                actorIt->second->setGlobalPose(*pxTransform);
            }
        }
    }

    auto PhysicsSystem::simulate(float fixedDelta) const -> void {
        if (!m_scene || !std::isfinite(fixedDelta) || fixedDelta <= 0.0f) {
            return;
        }
        m_scene->simulate(fixedDelta);
        m_scene->fetchResults(true);
    }

    auto PhysicsSystem::syncSceneFromActors(scene::Scene &scene) -> void {
        auto view = scene.view<components::TransformComponent, components::RigidbodyComponent, components::ColliderComponent>();
        for (auto handle : view) {
            auto isDynamic = view.get<components::RigidbodyComponent>(handle).dynamic;
            if (!isDynamic) {
                continue;
            }
            auto& transform = view.get<components::TransformComponent>(handle);
            auto actorIt = m_actors.find(handle);
            if (actorIt == m_actors.end()) {
                continue;
            }
            auto pxTransform = actorIt->second->getGlobalPose();
            transform.transform.position = fromPxVec3(pxTransform.p);
            transform.transform.rotation = fromPxQuat(pxTransform.q);
        }
    }

    auto PhysicsSystem::createMissingActors(scene::Scene &scene) -> void {
        auto view = scene.view<components::TransformComponent, components::RigidbodyComponent, components::ColliderComponent>();

        for (auto handle : view) {
            if (m_actors.size() >= MaxPhysicsActors) {
                break;
            }
            if (!m_actors.contains(handle)) {
                createActorForEntity(scene, {handle, &scene});
            }
        }

    }

    auto PhysicsSystem::createActorForEntity(scene::Scene &scene, scene::Entity entity) -> void {
        if (!m_physics || !m_scene || !m_defaultMaterial) {
            return;
        }
        auto& transform = scene.getComponent<components::TransformComponent>(entity);
        auto& rigidbody = scene.getComponent<components::RigidbodyComponent>(entity);
        auto& collider = scene.getComponent<components::ColliderComponent>(entity);

        const auto pxTransform = toValidPxTransform(transform);
        const auto pxCollider = PhysicsSystem::createGeometry(collider);
        if (!pxTransform || !pxCollider) {
            return;
        }

        physx::PxRigidActor* body = rigidbody.dynamic
            ? static_cast<physx::PxRigidActor*>(m_physics->createRigidDynamic(*pxTransform))
            : static_cast<physx::PxRigidActor*>(m_physics->createRigidStatic(*pxTransform));
        if (!body) {
            return;
        }

        physx::PxShape* shape = m_physics->createShape(pxCollider->any(), *m_defaultMaterial);
        if (!shape) {
            body->release();
            return;
        }
        if (collider.trigger) {
            shape->setFlag(physx::PxShapeFlag::eSIMULATION_SHAPE, false);
            shape->setFlag(physx::PxShapeFlag::eTRIGGER_SHAPE, true);
        }
        if (!body->attachShape(*shape)) {
            shape->release();
            body->release();
            return;
        }
        shape->release();

        if (rigidbody.dynamic) {
            auto* dynamicBody = static_cast<physx::PxRigidDynamic*>(body);
            if (rigidbody.useGravity) {
                dynamicBody->setActorFlag(physx::PxActorFlag::eDISABLE_GRAVITY, false);
            } else {
                dynamicBody->setActorFlag(physx::PxActorFlag::eDISABLE_GRAVITY, true);
            }
            if (!std::isfinite(rigidbody.mass) || rigidbody.mass <= 0.0f) {
                body->release();
                return;
            }
            dynamicBody->setMass(rigidbody.mass);
        }
        m_scene->addActor(*body);
        m_actors[entity.handle] = body;
    }

    auto PhysicsSystem::destroyRemovedActors(scene::Scene &scene) -> void {
        const auto& view = scene.view<components::TransformComponent, components::RigidbodyComponent, components::ColliderComponent>();
        for (auto it = m_actors.begin(); it != m_actors.end();) {
            if (!view.contains(it->first)) {
                m_scene->removeActor(*it->second);
                it->second->release();
                it = m_actors.erase(it);
            } else {
                ++it;
            }
        }
    }

    auto PhysicsSystem::createGeometry(const components::ColliderComponent &collider) -> std::optional<physx::PxGeometryHolder> {
        constexpr float MaxColliderDimension = 100000.0f;
        const auto validDimension = [](float value) {
            return std::isfinite(value) && value > 0.0f && value <= MaxColliderDimension;
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
                if (!validDimension(collider.radius) || !validDimension(collider.height)) {
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

    PhysicsSystem::~PhysicsSystem() {
        shutdown();
    }
}
