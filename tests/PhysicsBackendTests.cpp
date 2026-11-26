#include <array>
#include <cmath>
#include <iostream>
#include <string_view>
#include <vector>
#include "logging/Logger.hpp"
#if defined(AETHER_PHYSICS_BACKEND_PHYSX)
#include <PxPhysicsAPI.h>
#endif

#include "physics/PhysicsBackendFactory.hpp"
#include "systems/PhysicsSystem.hpp"

namespace {
    auto expect(bool condition, std::string_view message, int& failures) -> void {
        if (!condition) {
            std::cerr << "FAIL: " << message << '\n';
            ++failures;
        }
    }

    auto boxCollider(float halfExtent = 0.5f) -> AetherEngine::components::ColliderComponent {
        return {
            .type = AetherEngine::components::ColliderType::Box,
            .size = {halfExtent, halfExtent, halfExtent}
        };
    }

    auto sphereCollider(float radius = 0.5f) -> AetherEngine::components::ColliderComponent {
        return {
            .type = AetherEngine::components::ColliderType::Sphere,
            .radius = radius
        };
    }

    auto createBody(AetherEngine::physics::IPhysicsBackend& backend, const AetherEngine::physics::BodyDesc& desc, int& failures)
        -> AetherEngine::physics::BodyHandle {
        const auto result = backend.createBody(desc);
        if (!result) {
            std::cerr << "Body creation failed: " << result.error().message << '\n';
            ++failures;
        }
        return result.value_or(AetherEngine::physics::BodyHandle{});
    }

    auto capsuleAxis(int& failures) -> void {
        namespace ae = AetherEngine;
        auto backend = ae::physics::createPhysicsBackend();
        if (!backend || !backend->init()) { expect(false, "capsule test initialization", failures); return; }
        ae::physics::BodyDesc capsule{.collider = {.type = ae::components::ColliderType::Capsule, .radius = 0.5f, .height = 4.0f}, .dynamic = false};
        const auto obstacle = createBody(*backend, capsule, failures);
        ae::physics::BodyDesc ball{.transform = {.position = {1, 3, 0}}, .collider = sphereCollider(0.25f)};
        const auto supported = createBody(*backend, ball, failures);
        ball.transform.position = {0, 3, 2};
        const auto unsupported = createBody(*backend, ball, failures);
        for (int i = 0; i < 120; ++i) backend->simulate(1.0f / 60.0f);
        expect(backend->getTransform(supported).position.y > 0.5f, "capsule extends along engine-local X", failures);
        expect(backend->getTransform(unsupported).position.y < -5.0f, "capsule does not extend along Z", failures);
        expect(backend->getTransform(obstacle).rotation == ae::math::Quat::identity(), "capsule adaptation must not rotate the entity", failures);
    }

#if defined(AETHER_PHYSICS_BACKEND_PHYSX)
    auto physxMassProperties(int& failures) -> void {
        namespace ae = AetherEngine;
        auto backend = ae::physics::createPhysicsBackend();
        if (!backend || !backend->init()) { expect(false, "inertia test initialization", failures); return; }
        physx::PxScene* scene = nullptr;
        PxGetPhysics().getScenes(&scene, 1);
        expect(scene != nullptr, "PhysX scene is available", failures);
        if (!scene) return;
        for (bool sensor : {false, true}) {
            ae::physics::BodyDesc desc{.collider = {.size = {1, 2, 3}, .trigger = sensor}, .mass = 6, .useGravity = false};
            const auto handle = createBody(*backend, desc, failures);
            physx::PxActor* actor = nullptr;
            scene->getActors(physx::PxActorTypeFlag::eRIGID_DYNAMIC, &actor, 1);
            auto* body = actor ? actor->is<physx::PxRigidDynamic>() : nullptr;
            expect(body != nullptr, "created PhysX dynamic body", failures);
            if (body) {
                const auto inertia = body->getMassSpaceInertiaTensor();
                expect(std::abs(body->getMass() - 6.0f) < 1.0e-4f &&
                    std::abs(inertia.x - 26.0f) < 1.0e-4f && std::abs(inertia.y - 20.0f) < 1.0e-4f &&
                    std::abs(inertia.z - 10.0f) < 1.0e-4f, "box and sensor inertia matches dimensions and mass", failures);
                body->setAngularDamping(0);
                body->addTorque({26, 0, 0}, physx::PxForceMode::eIMPULSE);
                backend->simulate(1.0f / 60.0f);
                expect(std::abs(body->getAngularVelocity().x - 1.0f) < 1.0e-4f,
                    "angular response uses the calculated inertia", failures);
            }
            backend->destroyBody(handle);
        }
    }
#endif

    auto physicsDiagnostics(int& failures) -> void {
        namespace ae = AetherEngine;
        ae::logging::Logger logger;
        auto physics = ae::systems::PhysicsSystem::createPhysicsSystem(&logger);
        if (!physics || !physics->init()) { expect(false, "diagnostics test initialization", failures); return; }
        auto scene = ae::scene::Scene::createScene("Diagnostics");
        const auto add = [&](int i) {
            const auto entity = scene->createEntity("Body " + std::to_string(i));
            scene->addComponent<ae::components::TransformComponent>(entity).transform.position.x = static_cast<float>(i) * 3.0f;
            scene->addComponent<ae::components::RigidbodyComponent>(entity).dynamic = false;
            scene->addComponent<ae::components::ColliderComponent>(entity);
            return entity;
        };
        const auto invalid = add(0);
        scene->getComponent<ae::components::ColliderComponent>(invalid).size.x = 0;
        for (int i = 0; i < 3; ++i) physics->fixedUpdate(*scene, 0);
        expect(physics->errors().size() == 1 && physics->errors()[0].entityName == "Body 0" &&
            physics->errors()[0].error.message.find("Collider.size") != std::string::npos,
            "invalid body identifies the entity and field", failures);
        const auto messages = logger.snapshot();
        expect(messages.size() == 1 && messages[0].category == "Physics" && messages[0].message.find("Body 0") != std::string::npos,
            "repeated body failure logs once to the console", failures);
        scene->getComponent<ae::components::ColliderComponent>(invalid).size.x = 0.5f;
        physics->fixedUpdate(*scene, 0);
        expect(physics->errors().empty(), "correcting invalid geometry retries creation", failures);
        std::vector<ae::scene::Entity> entities{invalid};
        for (int i = 1; i <= 512; ++i) entities.push_back(add(i));
        physics->fixedUpdate(*scene, 0);
        expect(physics->errors().size() == 1 && physics->errors()[0].error.message.find("512") != std::string::npos,
            "actor limit reports the uncreated entity", failures);
        if (physics->errors().size() == 1) {
            const auto failedId = physics->errors()[0].entityId;
            const auto victim = scene->getComponent<ae::components::IdentityComponent>(entities[0]).id != failedId ? entities[0] : entities[1];
            scene->destroyEntity(victim);
            physics->fixedUpdate(*scene, 0);
            expect(physics->errors().empty(), "freeing an actor slot retries the skipped entity", failures);
        }
        physics->clearSceneBodies();
        expect(physics->errors().empty(), "clearing scene bodies clears diagnostics", failures);
    }

    auto physicsSystemCollisionEnabled(int& failures) -> void {
        namespace ae = AetherEngine;
        auto physics = ae::systems::PhysicsSystem::createPhysicsSystem();
        if (!physics || !physics->init()) {
            expect(false, "initialize PhysicsSystem for collisionEnabled", failures);
            return;
        }

        const auto run = [&](bool cubeEnabled, bool floorEnabled, bool sensor, const char* expectation) {
            auto scene = ae::scene::Scene::createScene("Collision enabled");
            const auto floor = scene->createEntity("Floor");
            scene->addComponent<ae::components::TransformComponent>(floor).transform.position.y = -1.0f;
            scene->addComponent<ae::components::RigidbodyComponent>(floor).dynamic = false;
            scene->addComponent<ae::components::ColliderComponent>(floor,
                ae::components::ColliderComponent{.size = {10, 1, 10}, .collisionEnabled = floorEnabled});

            const auto cube = scene->createEntity("Cube");
            scene->addComponent<ae::components::TransformComponent>(cube).transform.position.y = 3.0f;
            scene->addComponent<ae::components::RigidbodyComponent>(cube);
            scene->addComponent<ae::components::ColliderComponent>(cube,
                ae::components::ColliderComponent{.size = {0.5f, 0.5f, 0.5f}, .trigger = sensor, .collisionEnabled = cubeEnabled});

            constexpr float dt = 1.0f / 60.0f;
            const auto& pose = scene->getComponent<ae::components::TransformComponent>(cube).transform;
            for (int i = 0; i < 240; ++i) physics->fixedUpdate(*scene, dt);
            const float y = pose.position.y;
            physics->fixedUpdate(*scene, dt);
            const float nextY = pose.position.y;
            expect(physics->errors().empty(), "collisionEnabled scene has no body creation errors", failures);
            if (cubeEnabled && floorEnabled) {
                // Floor top at Y=0, cube half-height 0.5. Allow SDK contact tolerances.
                expect(std::abs(y - 0.5f) < 0.05f && std::abs(nextY - y) < 0.01f, expectation, failures);
            } else {
                expect(y < -2.5f && nextY < y, expectation, failures);
            }
            physics->clearSceneBodies();
        };

        run(true, true, false, "cube comes to rest on the enabled floor");
        run(false, true, false, "collision-disabled cube passes through the floor and continues falling");
        run(true, false, false, "cube passes through a collision-disabled floor and continues falling");
        run(false, true, true, "collision-disabled cube with trigger set continues falling through the floor");
    }

    auto physicsSystemFreeFall(int& failures) -> void {
        namespace ae = AetherEngine;
        auto physics = ae::systems::PhysicsSystem::createPhysicsSystem();
        const bool initialized = physics && physics->init().has_value();
        expect(initialized, "initialize PhysicsSystem for free fall", failures);
        if (!initialized) return;

        // Main's rabbit and tilted cube: removing their initial intersections
        // with the ground must leave pure world-Y gravity, independent of shape
        // orientation and visual scale. Exercise the ECS/backend round trip.
        for (bool withFloor : {false, true}) {
            auto scene = ae::scene::Scene::createScene("Free fall");
            std::array<ae::math::Transform, 2> initial{};
            initial[0].position = {2.8f, withFloor ? 4.0f : -0.95f, 2.3f};
            initial[1].position = {0.0f, withFloor ? 4.0f : 0.0f, 0.0f};
            initial[1].rotation = ae::math::Quat::fromAxisAngleDegrees({1, 0, 0}, 17.0f);
            initial[1].scale = {2, 3, 4};
            const std::array entities{scene->createEntity("Rabbit"), scene->createEntity("Cube")};
            for (std::size_t i = 0; i < entities.size(); ++i) {
                scene->addComponent<ae::components::TransformComponent>(entities[i], ae::components::TransformComponent{.transform = initial[i]});
                scene->addComponent<ae::components::RigidbodyComponent>(entities[i]);
                auto collider = boxCollider(1.0f);
                if (i == 0) collider.size = {1.2f, 1.8f, 1.2f};
                scene->addComponent<ae::components::ColliderComponent>(entities[i], collider);
            }
            if (withFloor) {
                const auto ground = scene->createEntity("Ground");
                scene->addComponent<ae::components::TransformComponent>(ground,
                    ae::components::TransformComponent{.transform = {.position = {0, -1.5f, 0}}});
                scene->addComponent<ae::components::RigidbodyComponent>(ground, ae::components::RigidbodyComponent{.dynamic = false});
                auto collider = boxCollider();
                collider.size = {12, 0.5f, 6};
                scene->addComponent<ae::components::ColliderComponent>(ground, collider);
            }
            // Recreating scene bodies must also reset their falling velocity.
            std::array<ae::math::Vec3, 2> firstRunPositions{};
            for (int run = 0; run < 2; ++run) {
                for (std::size_t i = 0; i < entities.size(); ++i)
                    scene->getComponent<ae::components::TransformComponent>(entities[i]).transform = initial[i];
                const int steps = withFloor ? 30 : 120; // Raised shapes have not reached the floor after 0.5 s.
                for (int step = 0; step < steps; ++step) physics->fixedUpdate(*scene, 1.0f / 60.0f);
                for (std::size_t i = 0; i < entities.size(); ++i) {
                    const auto& result = scene->getComponent<ae::components::TransformComponent>(entities[i]).transform;
                    expect(std::abs(result.position.x - initial[i].position.x) < 1.0e-4f &&
                           std::abs(result.position.z - initial[i].position.z) < 1.0e-4f,
                        "unobstructed PhysicsSystem fall has no lateral velocity", failures);
                    expect(result.position.y < initial[i].position.y - 0.5f, "PhysicsSystem gravity points down", failures);
                    if (run == 0) firstRunPositions[i] = result.position;
                    else expect((result.position - firstRunPositions[i]).length() < 1.0e-4f,
                        "recreating physics bodies resets falling velocity", failures);
                    expect(result.scale == initial[i].scale, "physics synchronization preserves visual scale", failures);
                }
                physics->clearSceneBodies();
            }
        }
    }
}

int main() {
    int failures = 0;
    auto backend = AetherEngine::physics::createPhysicsBackend();
    expect(static_cast<bool>(backend), "create selected physics backend", failures);
    if (!backend) {
        return 1;
    }

    const auto initialized = backend->init();
    if (!initialized) {
        std::cerr << "Backend initialization error: " << initialized.error().message << '\n';
    }
    expect(initialized.has_value(), "initialize selected physics backend", failures);
    if (!initialized) {
        return 1;
    }
    expect(backend->init().has_value(), "backend initialization is idempotent", failures);

    AetherEngine::physics::BodyDesc floorDesc{
        .transform = {
            .position = {0.0f, -1.0f, 0.0f}
        },
        .collider = boxCollider(1.0f),
        .dynamic = false
    };
    floorDesc.collider.size = {10.0f, 1.0f, 10.0f};
    const auto floor = createBody(*backend, floorDesc, failures);
    expect(floor.isValid(), "create static box", failures);

    const AetherEngine::physics::BodyDesc fallingDesc{
        .transform = {
            .position = {0.0f, 3.0f, 0.0f}
        },
        .collider = sphereCollider(),
        .mass = 2.0f,
        .dynamic = true,
        .useGravity = true
    };
    const auto fallingBody = createBody(*backend, fallingDesc, failures);
    expect(fallingBody.isValid(), "create dynamic sphere", failures);

    const AetherEngine::physics::BodyDesc floatingDesc{
        .transform = {
            .position = {2.0f, 3.0f, 0.0f}
        },
        .collider = {
            .type = AetherEngine::components::ColliderType::Capsule,
            .radius = 0.25f,
            .height = 1.0f
        },
        .mass = 1.0f,
        .dynamic = true,
        .useGravity = false
    };
    const auto floatingBody = createBody(*backend, floatingDesc, failures);
    expect(floatingBody.isValid(), "create gravity-free dynamic capsule", failures);

    AetherEngine::physics::BodyDesc triggerDesc{
        .transform = {
            .position = {-2.0f, 1.0f, 0.0f}
        },
        .collider = sphereCollider(),
        .dynamic = false
    };
    triggerDesc.collider.trigger = true;
    const auto trigger = createBody(*backend, triggerDesc, failures);
    expect(trigger.isValid(), "create trigger body", failures);

    AetherEngine::physics::BodyDesc invalidMassDesc = fallingDesc;
    invalidMassDesc.mass = 0.0f;
    expect(
        !backend->createBody(invalidMassDesc).has_value(),
        "reject invalid dynamic mass",
        failures
    );

    AetherEngine::physics::BodyDesc invalidColliderDesc = fallingDesc;
    invalidColliderDesc.collider.radius = -1.0f;
    expect(
        !backend->createBody(invalidColliderDesc).has_value(),
        "reject invalid collider dimensions",
        failures
    );

    constexpr float FixedDelta = 1.0f / 60.0f;
    for (int step = 0; step < 120; ++step) {
        backend->simulate(FixedDelta);
    }

    const auto fallingTransform = backend->getTransform(fallingBody);
    expect(
        fallingTransform.position.y < fallingDesc.transform.position.y - 0.5f,
        "dynamic body responds to gravity",
        failures
    );
    expect(
        fallingTransform.position.y > -0.1f,
        "dynamic body collides with static floor",
        failures
    );

    const auto floatingTransform = backend->getTransform(floatingBody);
    expect(
        std::abs(floatingTransform.position.y - floatingDesc.transform.position.y) < 1.0e-3f,
        "per-body gravity can be disabled",
        failures
    );

    const AetherEngine::math::Transform movedFloor{
        .position = {0.0f, -2.0f, 0.0f},
        .rotation = AetherEngine::math::Quat::identity()
    };
    backend->setTransform(floor, movedFloor);
    const auto floorTransform = backend->getTransform(floor);
    expect(
        std::abs(floorTransform.position.y - movedFloor.position.y) < 1.0e-4f,
        "set and get body transform",
        failures
    );

    backend->destroyBody(trigger);
    backend->destroyBody(floatingBody);
    backend->destroyBody(fallingBody);
    backend->destroyBody(floor);
    backend->shutdown();
    backend->shutdown();

    const auto reinitialized = backend->init();
    expect(reinitialized.has_value(), "backend can be reinitialized after shutdown", failures);
    backend->shutdown();

    physicsSystemFreeFall(failures);
    physicsSystemCollisionEnabled(failures);
    capsuleAxis(failures);
    physicsDiagnostics(failures);
#if defined(AETHER_PHYSICS_BACKEND_PHYSX)
    physxMassProperties(failures);
#endif

    if (failures == 0) {
        std::cout << "Physics backend regression tests passed\n";
    }
    return failures == 0 ? 0 : 1;
}
