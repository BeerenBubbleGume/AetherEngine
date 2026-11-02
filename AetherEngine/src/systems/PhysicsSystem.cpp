//
// Created by drhaz on 03.07.2026.
//

#include "systems/PhysicsSystem.hpp"

#include "physics/IPhysicsBackend.hpp"
#include "physics/PhysicsBackendFactory.hpp"
#include "logging/Logger.hpp"

namespace AetherEngine::systems {
    constexpr std::size_t MaxPhysicsActors = 512;
    constexpr int ActorLimitError = 4;
    namespace {
        auto sameDescription(const physics::BodyDesc& a, const physics::BodyDesc& b) -> bool {
            return a.transform.position == b.transform.position && a.transform.rotation == b.transform.rotation &&
                   a.collider == b.collider && a.mass == b.mass && a.dynamic == b.dynamic && a.useGravity == b.useGravity;
        }
    }

    PhysicsSystem::PhysicsSystem(std::unique_ptr<physics::IPhysicsBackend> backend, logging::Logger* logger)
        : m_backend(std::move(backend)), m_logger(logger) {
    }

    PhysicsSystem::PhysicsSystemPtr PhysicsSystem::createPhysicsSystem(logging::Logger* logger) {
        auto backend = physics::createPhysicsBackend();
        if (!backend) {
            return {};
        }

        return PhysicsSystemPtr(
            new PhysicsSystem(std::move(backend), logger),
            PhysicsSystemDeleter{}
        );
    }

    auto PhysicsSystem::PhysicsSystemDeleter::operator()(
        PhysicsSystem* system
    ) const -> void {
        delete system;
    }

    auto PhysicsSystem::init() const -> std::expected<void, PhysicsError> {
        if (!m_backend) {
            return std::unexpected(
                PhysicsError{1, "Physics backend is not available"}
            );
        }
        return m_backend->init();
    }

    auto PhysicsSystem::shutdown() -> void {
        m_actors.clear();
        m_failedBodies.clear();
        m_errors.clear();
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
        m_errors.clear();
        for (const auto& [handle, failed] : m_failedBodies) m_errors.push_back(failed.diagnostic);
        syncActorsFromScene(scene);
        /*applyPendingForcesAndImpulses();*/

        if (m_backend) {
            m_backend->simulate(fixedDelta);
        }

        syncSceneFromActors(scene);
    }

    auto PhysicsSystem::clearSceneBodies() noexcept -> void {
        if (m_backend) {
            for (const auto handle: m_actors | std::views::values) {
                m_backend->destroyBody(handle);
            }
        }
        m_actors.clear();
        m_failedBodies.clear();
        m_errors.clear();
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
            if (!m_actors.contains(handle)) {
                createActorForEntity(scene, {.handle = handle, .scene = &scene});
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
        if (m_actors.size() >= MaxPhysicsActors) {
            reportCreationFailure(scene, entity, desc, {ActorLimitError, "Physics actor limit reached (512 bodies)"});
            return;
        }
        if (const auto failed = m_failedBodies.find(entity.handle); failed != m_failedBodies.end() &&
            failed->second.diagnostic.error.code != ActorLimitError && sameDescription(failed->second.description, desc)) {
            return;
        }
        const auto body = m_backend->createBody(desc);
        if (!body) {
            reportCreationFailure(scene, entity, desc, body.error());
            return;
        }
        try {
            m_actors.emplace(entity.handle, *body);
        } catch (...) {
            m_backend->destroyBody(*body);
            throw;
        }
        m_failedBodies.erase(entity.handle);
    }

    auto PhysicsSystem::reportCreationFailure(scene::Scene& scene, scene::Entity entity,
        const physics::BodyDesc& desc, PhysicsError error) -> void {
        const auto& identity = scene.getComponent<components::IdentityComponent>(entity);
        const auto previous = m_failedBodies.find(entity.handle);
        const bool changed = previous == m_failedBodies.end() || previous->second.diagnostic.error.code != error.code ||
            previous->second.diagnostic.error.message != error.message;
        if (changed && m_logger) {
            m_logger->error("Entity '" + identity.name + "' [" + scene::toString(identity.id) + "]: " + error.message, "Physics");
        }
        m_failedBodies.insert_or_assign(entity.handle, FailedBody{desc, {identity.id, identity.name, std::move(error)}});
    }

    auto PhysicsSystem::destroyRemovedActors(scene::Scene& scene) -> void {
        const auto& view = scene.view<
            components::TransformComponent,
            components::RigidbodyComponent,
            components::ColliderComponent
        >();
        std::erase_if(m_failedBodies, [&view](const auto& entry) { return !view.contains(entry.first); });
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
