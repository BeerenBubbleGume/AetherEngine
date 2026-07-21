//
// Created by drhaz on 03.07.2026.
//

#include "systems/PhysicsSystem.hpp"

namespace engine::systems {
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
            const physx::PxTransform pxTransform{
                toPxVec3(transform.transform.position),
                toPxQuat(transform.transform.rotation)
            };
            actorIt->second->setGlobalPose(pxTransform);
        }
    }

    auto PhysicsSystem::simulate(float fixedDelta) const -> void {
        if (!m_scene || fixedDelta <= 0.0f) {
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
            if (!m_actors.contains(handle)) {
                createActorForEntity(scene, {handle, &scene});
            }
        }

    }

    auto PhysicsSystem::createActorForEntity(scene::Scene &scene, scene::Entity entity) -> void {
        auto& transform = scene.getComponent<components::TransformComponent>(entity);
        auto& rigidbody = scene.getComponent<components::RigidbodyComponent>(entity);
        auto& collider = scene.getComponent<components::ColliderComponent>(entity);

        physx::PxTransform pxTransform = physx::PxTransform(toPxVec3(transform.transform.position), toPxQuat(transform.transform.rotation));
        if (rigidbody.dynamic) {
            auto body = m_physics->createRigidDynamic(pxTransform);
            auto pxCollider = PhysicsSystem::createGeometry(collider);
            if (!pxCollider) {
                return;
            }
            if (rigidbody.useGravity) {
                body->setActorFlag(physx::PxActorFlag::eDISABLE_GRAVITY, false);
            } else {
                body->setActorFlag(physx::PxActorFlag::eDISABLE_GRAVITY, true);
            }
            physx::PxShape* shape;
            if (collider.trigger) {
                shape = m_physics->createShape(pxCollider->any(), *m_defaultMaterial);
                shape->setFlag(physx::PxShapeFlag::eSIMULATION_SHAPE, true);
                shape->setFlag(physx::PxShapeFlag::eTRIGGER_SHAPE, true);
            } else {
                shape = m_physics->createShape(pxCollider->any(), *m_defaultMaterial);
            }

            body->attachShape(*shape);
            shape->release();
            body->setMass(rigidbody.mass);
            body->setGlobalPose(pxTransform);
            m_scene->addActor(*body);
            m_actors[entity.handle] = body;
        } else {
            auto body = m_physics->createRigidStatic(pxTransform);
            auto pxCollider = PhysicsSystem::createGeometry(collider);
            if (!pxCollider) {
                return;
            }
            if (rigidbody.useGravity) {
                body->setActorFlag(physx::PxActorFlag::eDISABLE_GRAVITY, false);
            } else {
                body->setActorFlag(physx::PxActorFlag::eDISABLE_GRAVITY, true);
            }
            physx::PxShape* shape;
            if (collider.trigger) {
                shape = m_physics->createShape(pxCollider->any(), *m_defaultMaterial);
                shape->setFlag(physx::PxShapeFlag::eSIMULATION_SHAPE, true);
                shape->setFlag(physx::PxShapeFlag::eTRIGGER_SHAPE, true);
            } else {
                shape = m_physics->createShape(pxCollider->any(), *m_defaultMaterial);
            }
            body->attachShape(*shape);
            shape->release();
            body->setGlobalPose(pxTransform);
            m_scene->addActor(*body);
            m_actors[entity.handle] = body;
        }

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
        switch (collider.type) {
            case components::ColliderType::Box:
                return physx::PxGeometryHolder{
                    physx::PxBoxGeometry(
                        collider.size.x,
                        collider.size.y,
                        collider.size.z
                    )
                };

            case components::ColliderType::Sphere:
                return physx::PxGeometryHolder{
                    physx::PxSphereGeometry(collider.radius)
                };

            case components::ColliderType::Capsule:
                return physx::PxGeometryHolder{
                    physx::PxCapsuleGeometry(
                        collider.radius,
                        collider.height * 0.5f
                    )
                };
        }

        return std::nullopt;
    }

    PhysicsSystem::~PhysicsSystem() {
        shutdown();
    }
}
