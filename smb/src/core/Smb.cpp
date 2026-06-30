//
// Created by drhaz on 22.06.2026.
//

#include "core/Smb.hpp"

#include "components/ICameraComponent.hpp"
#include "components/IMaterialComponent.hpp"
#include "components/IMeshComponent.hpp"
#include "components/ITransformComponent.hpp"
#include "math/UTypes.hpp"

namespace smb {
    namespace {
        using engine::math::TQuat;
        using engine::math::TVec3;
    }

    auto SMB::init(engine::core::EngineContext &ctx) -> std::expected<void, engine::core::EngineError> {
#ifdef WIN32
        auto program = ctx.resources.loadProgram("basic", (ctx.paths.shadersRoot/"bin/win32/basic_vs.bin").string(),
            (ctx.paths.shadersRoot/"bin/win32/basic_fs.bin").string());
#elif __APPLE__
        auto program = ctx.resources.loadProgram("basic", (ctx.paths.shadersRoot/"bin/osx_arm/basic_vs.bin").string(),
            (ctx.paths.shadersRoot/"bin/osx_arm/basic_fs.bin").string());
#elif __linux__
        auto program = ctx.resources.loadProgram("basic", (ctx.paths.shadersRoot/"bin/osx_arm/basic_vs.bin").string(),
            (ctx.paths.shadersRoot/"bin/osx_arm/basic_fs.bin").string());
#endif
        if (!program.isValid()) {
            return std::unexpected(engine::core::EngineError{1, "Failed to load shader program"});
        }

        auto mesh = ctx.resources.loadMesh((ctx.paths.assetsRoot / "meshes/bin/bunny.bin").string());

        if (!mesh.isValid()) {
            return std::unexpected(engine::core::EngineError{1, "Failed to load bunny mesh"});
        }
        auto camera = ctx.scene.createEntity();

        ctx.scene.addComponent<engine::components::ITransformComponent>(camera, engine::components::ITransformComponent{
            .transform = {
                .position = {0.0f, 0.0f, 5.0f},
                .rotation = TQuat::identity(),
                .scale = TVec3::one()
            }
        });

        ctx.scene.addComponent<engine::components::ICameraComponent>(camera, engine::components::ICameraComponent{
            .fovYDegrees = 60.0f,
            .nearPlane = 0.1f,
            .farPlane = 100.0f
        });

        ctx.scene.setActiveCamera(camera);

        auto player = ctx.scene.createEntity();
        ctx.scene.addComponent<engine::components::ITransformComponent>(player, engine::components::ITransformComponent {
            .transform = engine::graphics::Transform{
                .position = {0.0f, 0.0f, 0.0f},
                .rotation = TQuat::identity(),
                .scale = {1.0f, 1.0f, 1.0f}
            }
        });
        ctx.scene.addComponent<engine::components::IMeshComponent>(player, engine::components::IMeshComponent {
            .mesh = mesh,
        });
        ctx.scene.addComponent<engine::components::IMaterialComponent>(player, engine::components::IMaterialComponent {
            .program = {
                .program = program,
                .baseColor = {0.0f, 0.0f, 1.0f, 1.0f}
            }
        });

        auto secondBunny = ctx.scene.createEntity();
        ctx.scene.addComponent<engine::components::ITransformComponent>(secondBunny, engine::components::ITransformComponent {
            .transform = engine::graphics::Transform{
                .position = {-5.0f, -5.0f, -5.0f},
                .rotation = TQuat::fromAxisAngleDegrees({0.0f, 1.0f, 0.0f}, 30.0f) *
                            TQuat::fromAxisAngleDegrees({1.0f, 0.0f, 0.0f}, 40.0f),
                .scale = {2.0f, 2.0f, 2.0f}
            },
        });
        ctx.scene.addComponent<engine::components::IMeshComponent>(secondBunny, engine::components::IMeshComponent {
            .mesh = mesh,
        });
        ctx.scene.addComponent<engine::components::IMaterialComponent>(secondBunny, engine::components::IMaterialComponent {
            .program = {
                .program = program,
                .baseColor = {1.0f, 0.0f, 0.0f, 1.0f}
            }
        });

        playerEntity = player;
        ctx.input.bindKey(engine::systems::SYInputAction::MoveLeft, engine::systems::SYKey::A);
        ctx.input.bindKey(engine::systems::SYInputAction::MoveRight, engine::systems::SYKey::D);
        ctx.input.bindKey(engine::systems::SYInputAction::MoveForward, engine::systems::SYKey::W);
        ctx.input.bindKey(engine::systems::SYInputAction::MoveBackward, engine::systems::SYKey::S);
        ctx.input.bindKey(engine::systems::SYInputAction::RotateLeft, engine::systems::SYKey::Left);
        ctx.input.bindKey(engine::systems::SYInputAction::RotateRight, engine::systems::SYKey::Right);
        ctx.input.bindKey(engine::systems::SYInputAction::LookUp, engine::systems::SYKey::Up);
        ctx.input.bindKey(engine::systems::SYInputAction::LookDown, engine::systems::SYKey::Down);
        ctx.input.bindKey(engine::systems::SYInputAction::Reset, engine::systems::SYKey::R);
        ctx.input.bindKey(engine::systems::SYInputAction::Quit, engine::systems::SYKey::Escape);

        return {};
    }

    auto SMB::update(float dt, engine::core::EngineContext &ctx) -> void {
        if (!playerEntity) {
            return;
        }
        auto& object = ctx.scene.getComponent<engine::components::ITransformComponent>(*playerEntity);

        auto& t = object.transform;

        const float speed = 1.0f * dt;
        const float angularSpeedDegrees = 90.0f * dt;

        if (ctx.input.isActionDown(engine::systems::SYInputAction::MoveLeft)) t.position.x += speed;
        if (ctx.input.isActionDown(engine::systems::SYInputAction::MoveRight)) t.position.x -= speed;
        if (ctx.input.isActionDown(engine::systems::SYInputAction::MoveForward)) t.position.y += speed;
        if (ctx.input.isActionDown(engine::systems::SYInputAction::MoveBackward)) t.position.y -= speed;

        TQuat delta = TQuat::identity();

        if (ctx.input.isActionDown(engine::systems::SYInputAction::RotateLeft)) {
            delta = TQuat::fromAxisAngleDegrees({0.0f, 1.0f, 0.0f}, -angularSpeedDegrees) * delta;
        }
        if (ctx.input.isActionDown(engine::systems::SYInputAction::RotateRight)) {
            delta = TQuat::fromAxisAngleDegrees({0.0f, 1.0f, 0.0f}, angularSpeedDegrees) * delta;
        }
        if (ctx.input.isActionDown(engine::systems::SYInputAction::LookUp)) {
            delta = TQuat::fromAxisAngleDegrees({1.0f, 0.0f, 0.0f}, -angularSpeedDegrees) * delta;
        }
        if (ctx.input.isActionDown(engine::systems::SYInputAction::LookDown)) {
            delta = TQuat::fromAxisAngleDegrees({1.0f, 0.0f, 0.0f}, angularSpeedDegrees) * delta;
        }

        t.rotation = (delta * t.rotation).normalized();

        if (ctx.input.isMouseButtonDown(engine::systems::SYMouseButton::Right)) {
            constexpr float sensitivity = 0.1f;

            const float yaw = ctx.input.mouseDeltaX() * sensitivity;
            const float pitch = ctx.input.mouseDeltaY() * sensitivity;

            const auto mouseDelta =
                TQuat::fromAxisAngleDegrees({0.0f, 1.0f, 0.0f}, yaw) *
                TQuat::fromAxisAngleDegrees({1.0f, 0.0f, 0.0f}, pitch);

            t.rotation = (mouseDelta * t.rotation).normalized();
        }
        if (ctx.input.mouseWheelY() != 0.0f) {
            auto& camera = ctx.scene.getComponent<engine::components::ICameraComponent>(
                ctx.scene.getActiveCamera()
            );

            camera.fovYDegrees -= ctx.input.mouseWheelY() * 2.0f;
            camera.fovYDegrees = std::clamp(camera.fovYDegrees, 20.0f, 100.0f);
        }
        if (ctx.input.wasActionPressed(engine::systems::SYInputAction::Reset)) {
            t.reset();
        }

        if (ctx.input.wasActionPressed(engine::systems::SYInputAction::Quit)) {
            ctx.requestQuit();
        }
    }

    auto SMB::run() -> void {
        // This function is intentionally left empty, as the main loop is handled by the engine.
    }

    auto SMB::createSMB() -> SMBPtr {
        return SMBPtr(new SMB(), SMBDeleter{});
    }


    SMB::~SMB() {
    }
} // smb
