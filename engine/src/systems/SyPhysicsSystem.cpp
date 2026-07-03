//
// Created by drhaz on 03.07.2026.
//

#include "systems/SyPhysicsSystem.hpp"

namespace engine::systems {
    static auto toPxVec3(engine::math::TVec3 v) -> physx::PxVec3 {
        return {v.x, v.y, v.z};
    }

    static auto toPxQuat(engine::math::TQuat q) -> physx::PxQuat {
        return {q.x, q.y, q.z, q.w};
    }

    static auto fromPxVec3(const physx::PxVec3& v) -> engine::math::TVec3 {
        return {v.x, v.y, v.z};
    }

    static auto fromPxQuat(const physx::PxQuat& q) -> engine::math::TQuat {
        return {q.w, q.x, q.y, q.z};
    }

    SYPhysicsSystem::SYPhysicsSystemPtr SYPhysicsSystem::createPhysicsSystem() {
        return SYPhysicsSystemPtr(new SYPhysicsSystem(), SYPhysicsSystemDeleter{});
    }

    auto SYPhysicsSystem::SYPhysicsSystemDeleter::operator()(SYPhysicsSystem* system) const -> void {
        delete system;
    }

    auto SYPhysicsSystem::init() -> std::expected<void, SYPhysicsError> {
        const physx::PxTolerancesScale scale{1.0f, 10.0f};

        m_foundation = PxCreateFoundation(PX_PHYSICS_VERSION, m_allocator, m_errorCallback);
        if (!m_foundation) {
            return std::unexpected(SYPhysicsError{1, "Failed to create PhysX foundation"});
        }

        m_physics = PxCreatePhysics(PX_PHYSICS_VERSION, *m_foundation, scale, true);
        if (!m_physics) {
            return std::unexpected(SYPhysicsError{1, "Failed to create PhysX physics"});
        }

        m_dispatcher = physx::PxDefaultCpuDispatcherCreate(1);
        if (!m_dispatcher) {
            return std::unexpected(SYPhysicsError{1, "Failed to create PhysX dispatcher"});
        }

        physx::PxSceneDesc sceneDesc{scale};
        sceneDesc.gravity = physx::PxVec3{0.0f, -9.81f, 0.0f};
        sceneDesc.cpuDispatcher = m_dispatcher;
        sceneDesc.filterShader = physx::PxDefaultSimulationFilterShader;

        if (!sceneDesc.isValid()) {
            return std::unexpected(SYPhysicsError{1, "Invalid PhysX scene descriptor"});
        }

        m_scene = m_physics->createScene(sceneDesc);
        if (!m_scene) {
            return std::unexpected(SYPhysicsError{1, "Failed to create PhysX scene"});
        }

        m_defaultMaterial = m_physics->createMaterial(0.5f, 0.5f, 0.5f);
        if (!m_defaultMaterial) {
            return std::unexpected(SYPhysicsError{1, "Failed to create PhysX material"});
        }

        return {};
    }

    auto SYPhysicsSystem::shutdown() -> void {
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

    auto SYPhysicsSystem::fixedUpdate(scene::SCScene &scene, float fixedDelta) -> void {
        destroyRemovedActors(scene);
        createMissingActors(scene);
        syncActorsFromScene(scene);
        /*applyPendingForcesAndImpulses();*/

        simulate(fixedDelta);

        syncSceneFromActors(scene);
    }

    auto SYPhysicsSystem::syncActorsFromScene(scene::SCScene &scene) -> void {
        auto view = scene.view<components::ITransformComponent, components::IRigidbodyComponent, components::IColliderComponent>();
        for (auto handle : view) {
            auto actorIt = m_actors.find(handle);
            if (actorIt == m_actors.end()) {
                continue;
            }
            const auto& rigidbody = view.get<components::IRigidbodyComponent>(handle);
            if (rigidbody.dynamic) {
                continue;
            }
            const auto& transform = view.get<components::ITransformComponent>(handle);
            const physx::PxTransform pxTransform{
                toPxVec3(transform.transform.position),
                toPxQuat(transform.transform.rotation)
            };
            actorIt->second->setGlobalPose(pxTransform);
        }
    }

    auto SYPhysicsSystem::simulate(float fixedDelta) const -> void {
        if (!m_scene || fixedDelta <= 0.0f) {
            return;
        }
        m_scene->simulate(fixedDelta);
        m_scene->fetchResults(true);
    }

    auto SYPhysicsSystem::syncSceneFromActors(scene::SCScene &scene) -> void {
        auto view = scene.view<components::ITransformComponent, components::IRigidbodyComponent, components::IColliderComponent>();
        for (auto handle : view) {
            auto isDynamic = view.get<components::IRigidbodyComponent>(handle).dynamic;
            if (!isDynamic) {
                continue;
            }
            auto& transform = view.get<components::ITransformComponent>(handle);
            auto actorIt = m_actors.find(handle);
            if (actorIt == m_actors.end()) {
                continue;
            }
            auto pxTransform = actorIt->second->getGlobalPose();
            transform.transform.position = fromPxVec3(pxTransform.p);
            transform.transform.rotation = fromPxQuat(pxTransform.q);
        }
    }

    auto SYPhysicsSystem::createMissingActors(scene::SCScene &scene) -> void {
        auto view = scene.view<components::ITransformComponent, components::IRigidbodyComponent, components::IColliderComponent>();

        for (auto handle : view) {
            if (!m_actors.contains(handle)) {
                createActorForEntity(scene, {handle, &scene});
            }
        }

    }

    auto SYPhysicsSystem::createActorForEntity(scene::SCScene &scene, scene::Entity entity) -> void {
        auto& transform = scene.getComponent<components::ITransformComponent>(entity);
        auto& rigidbody = scene.getComponent<components::IRigidbodyComponent>(entity);
        auto& collider = scene.getComponent<components::IColliderComponent>(entity);

        physx::PxTransform pxTransform = physx::PxTransform(toPxVec3(transform.transform.position), toPxQuat(transform.transform.rotation));
        if (rigidbody.dynamic) {
            auto body = m_physics->createRigidDynamic(pxTransform);
            auto pxCollider = SYPhysicsSystem::createGeometry(collider);
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
            auto pxCollider = SYPhysicsSystem::createGeometry(collider);
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

    auto SYPhysicsSystem::destroyRemovedActors(scene::SCScene &scene) -> void {
        const auto& view = scene.view<components::ITransformComponent, components::IRigidbodyComponent, components::IColliderComponent>();
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

    auto SYPhysicsSystem::createGeometry(const components::IColliderComponent &collider) -> std::optional<physx::PxGeometryHolder> {
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

    SYPhysicsSystem::~SYPhysicsSystem() {
        shutdown();
    }
}
