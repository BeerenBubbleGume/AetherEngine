#include <cmath>
#include <iostream>
#include <string_view>

#include "physics/PhysicsBackendFactory.hpp"

namespace {
    auto expect(bool condition, std::string_view message, int& failures) -> void {
        if (!condition) {
            std::cerr << "FAIL: " << message << '\n';
            ++failures;
        }
    }

    auto boxCollider(float halfExtent = 0.5f) -> engine::components::ColliderComponent {
        return {
            .type = engine::components::ColliderType::Box,
            .size = {halfExtent, halfExtent, halfExtent}
        };
    }

    auto sphereCollider(float radius = 0.5f) -> engine::components::ColliderComponent {
        return {
            .type = engine::components::ColliderType::Sphere,
            .radius = radius
        };
    }
}

int main() {
    int failures = 0;
    auto backend = engine::physics::createPhysicsBackend();
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

    engine::physics::BodyDesc floorDesc{
        .transform = {
            .position = {0.0f, -1.0f, 0.0f}
        },
        .collider = boxCollider(1.0f),
        .dynamic = false
    };
    floorDesc.collider.size = {10.0f, 1.0f, 10.0f};
    const auto floor = backend->createBody(floorDesc);
    expect(floor.isValid(), "create static box", failures);

    const engine::physics::BodyDesc fallingDesc{
        .transform = {
            .position = {0.0f, 3.0f, 0.0f}
        },
        .collider = sphereCollider(),
        .mass = 2.0f,
        .dynamic = true,
        .useGravity = true
    };
    const auto fallingBody = backend->createBody(fallingDesc);
    expect(fallingBody.isValid(), "create dynamic sphere", failures);

    const engine::physics::BodyDesc floatingDesc{
        .transform = {
            .position = {2.0f, 3.0f, 0.0f}
        },
        .collider = {
            .type = engine::components::ColliderType::Capsule,
            .radius = 0.25f,
            .height = 1.0f
        },
        .mass = 1.0f,
        .dynamic = true,
        .useGravity = false
    };
    const auto floatingBody = backend->createBody(floatingDesc);
    expect(floatingBody.isValid(), "create gravity-free dynamic capsule", failures);

    engine::physics::BodyDesc triggerDesc{
        .transform = {
            .position = {-2.0f, 1.0f, 0.0f}
        },
        .collider = sphereCollider(),
        .dynamic = false
    };
    triggerDesc.collider.trigger = true;
    const auto trigger = backend->createBody(triggerDesc);
    expect(trigger.isValid(), "create trigger body", failures);

    engine::physics::BodyDesc invalidMassDesc = fallingDesc;
    invalidMassDesc.mass = 0.0f;
    expect(
        !backend->createBody(invalidMassDesc).isValid(),
        "reject invalid dynamic mass",
        failures
    );

    engine::physics::BodyDesc invalidColliderDesc = fallingDesc;
    invalidColliderDesc.collider.radius = -1.0f;
    expect(
        !backend->createBody(invalidColliderDesc).isValid(),
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

    const engine::math::Transform movedFloor{
        .position = {0.0f, -2.0f, 0.0f},
        .rotation = engine::math::Quat::identity()
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

    if (failures == 0) {
        std::cout << "Physics backend regression tests passed\n";
    }
    return failures == 0 ? 0 : 1;
}
