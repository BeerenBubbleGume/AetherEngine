//
// Created by drhaz on 03.07.2026.
//

#include "systems/PhysicsSystem.hpp"

#include "physics/IPhysicsBackend.hpp"
#include "physics/PhysicsBackendFactory.hpp"

namespace AetherEngine::systems {
    constexpr std::size_t MaxPhysicsActors = 512;

    PhysicsSystem::PhysicsSystem(std::unique_ptr<physics::IPhysicsBackend> backend)
        : m_backend(std::move(backend)) {
    }

    PhysicsSystem::PhysicsSystemPtr PhysicsSystem::createPhysicsSystem() {
        auto backend = physics::createPhysicsBackend();
        if (!backend) {
            return {};
        }

        return PhysicsSystemPtr(
            new PhysicsSystem(std::move(backend)),
            PhysicsSystemDeleter{}
        );
    }

    auto PhysicsSystem::PhysicsSystemDeleter::operator()(
        PhysicsSystem* system
    ) const -> void {
        delete system;
    }

    auto PhysicsSystem::init() -> std::expected<void, PhysicsError> {
        if (!m_backend) {
            return std::unexpected(
                PhysicsError{1, "Physics backend is not available"}
            );
        }
        return m_backend->init();
    }

    auto PhysicsSystem::shutdown() -> void {
        m_actors.clear();
        if (m_backend) {
            m_backend->shutdown();
        }
    }

    auto PhysicsSystem::fixedUpdate(
        scene::Scene& scene,
        float fixedDelta
    ) -> void {
        destroyRemovedActors(scene);
        createMissingActors(scene);
        syncActorsFromScene(scene);
        /*applyPendingForcesAndImpulses();*/

        if (m_backend) {
            m_backend->simulate(fixedDelta);
        }

        syncSceneFromActors(scene);
    }

    auto PhysicsSystem::syncActorsFromScene(scene::Scene& scene) -> void {
        if (!m_backend) {
            return;
        }

        auto view = scene.view<
            components::TransformComponent,
            components::RigidbodyComponent,
            components::ColliderComponent
        >();
        for (auto handle : view) {
            const auto actorIt = m_actors.find(handle);
            if (actorIt == m_actors.end()) {
                continue;
            }

            const auto& rigidbody =
                view.get<components::RigidbodyComponent>(handle);
            if (rigidbody.dynamic) {
                continue;
            }

            const auto& transform =
                view.get<components::TransformComponent>(handle);
            m_backend->setTransform(actorIt->second, transform.transform);
        }
    }

    auto PhysicsSystem::syncSceneFromActors(scene::Scene& scene) -> void {
        if (!m_backend) {
            return;
        }

        auto view = scene.view<
            components::TransformComponent,
            components::RigidbodyComponent,
            components::ColliderComponent
        >();
        for (auto handle : view) {
            const auto isDynamic =
                view.get<components::RigidbodyComponent>(handle).dynamic;
            if (!isDynamic) {
                continue;
            }

            auto& transform = view.get<components::TransformComponent>(handle);
            const auto actorIt = m_actors.find(handle);
            if (actorIt == m_actors.end()) {
                continue;
            }

            const auto physicsTransform =
                m_backend->getTransform(actorIt->second);
            transform.transform.position = physicsTransform.position;
            transform.transform.rotation = physicsTransform.rotation;
        }
    }

    auto PhysicsSystem::createMissingActors(scene::Scene& scene) -> void {
        auto view = scene.view<
            components::TransformComponent,
            components::RigidbodyComponent,
            components::ColliderComponent
        >();

        for (auto handle : view) {
            if (m_actors.size() >= MaxPhysicsActors) {
                break;
            }
            if (!m_actors.contains(handle)) {
                createActorForEntity(scene, {handle, &scene});
            }
        }
    }

    auto PhysicsSystem::createActorForEntity(
        scene::Scene& scene,
        scene::Entity entity
    ) -> void {
        if (!m_backend) {
            return;
        }

        const auto& transform =
            scene.getComponent<components::TransformComponent>(entity);
        const auto& rigidbody =
            scene.getComponent<components::RigidbodyComponent>(entity);
        const auto& collider =
            scene.getComponent<components::ColliderComponent>(entity);

        const physics::BodyDesc desc{
            .transform = transform.transform,
            .collider = collider,
            .mass = rigidbody.mass,
            .dynamic = rigidbody.dynamic,
            .useGravity = rigidbody.useGravity
        };
        const auto body = m_backend->createBody(desc);
        if (body.isValid()) {
            m_actors.emplace(entity.handle, body);
        }
    }

    auto PhysicsSystem::destroyRemovedActors(scene::Scene& scene) -> void {
        const auto& view = scene.view<
            components::TransformComponent,
            components::RigidbodyComponent,
            components::ColliderComponent
        >();
        for (auto actorIt = m_actors.begin(); actorIt != m_actors.end();) {
            if (!view.contains(actorIt->first)) {
                if (m_backend) {
                    m_backend->destroyBody(actorIt->second);
                }
                actorIt = m_actors.erase(actorIt);
            } else {
                ++actorIt;
            }
        }
    }

    PhysicsSystem::~PhysicsSystem() {
        shutdown();
    }
}
