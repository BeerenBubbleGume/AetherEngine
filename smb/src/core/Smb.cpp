//
// Created by drhaz on 22.06.2026.
//

#include "core/Smb.hpp"

#include <algorithm>
#include <filesystem>

#include "components/CameraComponent.hpp"
#include "components/IdentityComponent.hpp"
#include "components/MaterialComponent.hpp"
#include "components/MeshComponent.hpp"
#include "components/PhysicsComponents.hpp"
#include "components/TransformComponent.hpp"
#include "graphics/SceneView.hpp"
#include "math/CameraMatrices.hpp"
#include "math/Types.hpp"
#include "systems/RenderSystem.hpp"

namespace smb {
    namespace {
        using engine::math::Quat;
        using engine::math::Vec3;

        auto toBgfxClearFlags(uint8_t cameraFlags) -> uint16_t {
            uint16_t flags = 0;
            if ((cameraFlags & engine::components::ClearColor) != 0) {
                flags |= BGFX_CLEAR_COLOR;
            }
            if ((cameraFlags & engine::components::ClearDepth) != 0) {
                flags |= BGFX_CLEAR_DEPTH;
            }
            if ((cameraFlags & engine::components::ClearStencil) != 0) {
                flags |= BGFX_CLEAR_STENCIL;
            }
            return flags;
        }
    }

    auto SMB::init(engine::core::EngineContext &ctx) -> std::expected<void, engine::core::EngineError> {
        std::filesystem::path vertexShaderAssetPath;
        std::filesystem::path fragmentShaderAssetPath;

        ctx.input.bindKey(engine::systems::InputAction::MoveLeft, engine::systems::Key::A);
        ctx.input.bindKey(engine::systems::InputAction::MoveRight, engine::systems::Key::D);
        ctx.input.bindKey(engine::systems::InputAction::MoveForward, engine::systems::Key::W);
        ctx.input.bindKey(engine::systems::InputAction::MoveBackward, engine::systems::Key::S);
        ctx.input.bindKey(engine::systems::InputAction::RotateLeft, engine::systems::Key::Left);
        ctx.input.bindKey(engine::systems::InputAction::RotateRight, engine::systems::Key::Right);
        ctx.input.bindKey(engine::systems::InputAction::LookUp, engine::systems::Key::Up);
        ctx.input.bindKey(engine::systems::InputAction::LookDown, engine::systems::Key::Down);
        ctx.input.bindKey(engine::systems::InputAction::Reset, engine::systems::Key::R);
        ctx.input.bindKey(engine::systems::InputAction::Quit, engine::systems::Key::Escape);
        if (!ctx.scene.isEmpty()) {
            auto player = ctx.scene.findEntityById("player");
            if (!player) {
                return std::unexpected(engine::core::EngineError{1, "Failed to find player entity"});
            }
            if (!ctx.scene.hasComponent<engine::components::TransformComponent>(*player)) {
                return std::unexpected(engine::core::EngineError{1, "Player entity has no Transform component"});
            }
            playerEntity = player;
            return {};
        }
#ifdef WIN32
        vertexShaderAssetPath = "shaders/bin/win32/basic_vs.bin";
        fragmentShaderAssetPath = "shaders/bin/win32/basic_fs.bin";
#elif __APPLE__
        vertexShaderAssetPath = "shaders/bin/osx_arm/basic_vs.bin";
        fragmentShaderAssetPath = "shaders/bin/osx_arm/basic_fs.bin";
#elif __linux__
        vertexShaderAssetPath = "shaders/bin/osx_arm/basic_vs.bin";
        fragmentShaderAssetPath = "shaders/bin/osx_arm/basic_fs.bin";
#endif
        auto program = ctx.resources.loadProgram(
            "basic",
            vertexShaderAssetPath.generic_string(),
            fragmentShaderAssetPath.generic_string()
        );
        if (!program.isValid()) {
            return std::unexpected(engine::core::EngineError{1, "Failed to load shader program"});
        }
        const std::filesystem::path brickTextureAssetPath{"textures/bin/brick.ktx"};
        auto texture = ctx.resources.loadTexture(brickTextureAssetPath.generic_string());

        const std::filesystem::path bunnyMeshAssetPath{"meshes/bin/bunny.bin"};
        auto mesh = ctx.resources.loadMesh(bunnyMeshAssetPath.generic_string());

        if (!mesh.isValid()) {
            return std::unexpected(engine::core::EngineError{1, "Failed to load bunny mesh"});
        }
        auto camera = ctx.scene.createEntity();
        ctx.scene.addComponent<engine::components::IdentityComponent>(camera, engine::components::IdentityComponent{
            .id = "camera",
            .name = "Camera"
        });

        ctx.scene.addComponent<engine::components::TransformComponent>(camera, engine::components::TransformComponent{
            .transform = {
                .position = {0.0f, 0.0f, 5.0f},
                .rotation = Quat::identity(),
                .scale = Vec3::one()
            }
        });

        ctx.scene.addComponent<engine::components::CameraComponent>(camera, engine::components::CameraComponent{
            .fovYDegrees = 60.0f,
            .nearPlane = 0.1f,
            .farPlane = 100.0f
        });

        ctx.scene.setActiveCamera(camera);

        auto player = ctx.scene.createEntity();
        ctx.scene.addComponent<engine::components::IdentityComponent>(player, engine::components::IdentityComponent{
            .id = "player",
            .name = "Player"
        });
        ctx.scene.addComponent<engine::components::TransformComponent>(player, engine::components::TransformComponent {
            .transform = engine::graphics::Transform{
                .position = {0.0f, 0.0f, 0.0f},
                .rotation = Quat::identity(),
                .scale = {1.0f, 1.0f, 1.0f}
            }
        });
        ctx.scene.addComponent<engine::components::MeshComponent>(player, engine::components::MeshComponent {
            .mesh = mesh,
            .assetPath = bunnyMeshAssetPath.generic_string()
        });
        ctx.scene.addComponent<engine::components::MaterialComponent>(player, engine::components::MaterialComponent {
            .material = {
                .program = program,
                .baseColor = {0.0f, 0.0f, 1.0f, 1.0f},
                .textures = {texture}
            },
            .programName = "basic",
            .vertexShaderPath = vertexShaderAssetPath.generic_string(),
            .fragmentShaderPath = fragmentShaderAssetPath.generic_string(),
            .texturePaths = {brickTextureAssetPath.generic_string()}
        });

        auto secondBunny = ctx.scene.createEntity();
        ctx.scene.addComponent<engine::components::IdentityComponent>(secondBunny, engine::components::IdentityComponent{
            .id = "second_bunny",
            .name = "Second Bunny"
        });
        ctx.scene.addComponent<engine::components::TransformComponent>(secondBunny, engine::components::TransformComponent {
            .transform = engine::graphics::Transform{
                .position = {-5.0f, -5.0f, -5.0f},
                .rotation = Quat::fromAxisAngleDegrees({0.0f, 1.0f, 0.0f}, 30.0f) *
                            Quat::fromAxisAngleDegrees({1.0f, 0.0f, 0.0f}, 40.0f),
                .scale = {2.0f, 2.0f, 2.0f}
            },
        });
        ctx.scene.addComponent<engine::components::MeshComponent>(secondBunny, engine::components::MeshComponent {
            .mesh = mesh,
            .assetPath = bunnyMeshAssetPath.generic_string()
        });
        ctx.scene.addComponent<engine::components::MaterialComponent>(secondBunny, engine::components::MaterialComponent {
            .material = {
                .program = program,
                .baseColor = {1.0f, 0.0f, 0.0f, 1.0f},
                .textures = {texture}
            },
            .programName = "basic",
            .vertexShaderPath = vertexShaderAssetPath.generic_string(),
            .fragmentShaderPath = fragmentShaderAssetPath.generic_string(),
            .texturePaths = {brickTextureAssetPath.generic_string()}
        });
        ctx.scene.addComponent<engine::components::ColliderComponent>(secondBunny, engine::components::ColliderComponent {});
        ctx.scene.addComponent<engine::components::RigidbodyComponent>(secondBunny, engine::components::RigidbodyComponent {});
        playerEntity = player;
        auto serializeResult = ctx.sceneSerializer.serializeScene(ctx.scene);
        if (!serializeResult) {
            return std::unexpected(engine::core::EngineError{1, serializeResult.error().message});
        }
        return {};
    }

    auto SMB::update(float dt, engine::core::EngineContext &ctx) -> void {
        if (!playerEntity || !playerEntity->isValid() || playerEntity->scene != &ctx.scene ||
            !ctx.scene.hasComponent<engine::components::TransformComponent>(*playerEntity)) {
            return;
        }
        auto& object = ctx.scene.getComponent<engine::components::TransformComponent>(*playerEntity);

        auto& t = object.transform;

        const float speed = 1.0f * dt;
        const float angularSpeedDegrees = 90.0f * dt;

        if (ctx.input.isActionDown(engine::systems::InputAction::MoveLeft)) t.position.x += speed;
        if (ctx.input.isActionDown(engine::systems::InputAction::MoveRight)) t.position.x -= speed;
        if (ctx.input.isActionDown(engine::systems::InputAction::MoveForward)) t.position.y += speed;
        if (ctx.input.isActionDown(engine::systems::InputAction::MoveBackward)) t.position.y -= speed;

        Quat delta = Quat::identity();

        if (ctx.input.isActionDown(engine::systems::InputAction::RotateLeft)) {
            delta = Quat::fromAxisAngleDegrees({0.0f, 1.0f, 0.0f}, -angularSpeedDegrees) * delta;
        }
        if (ctx.input.isActionDown(engine::systems::InputAction::RotateRight)) {
            delta = Quat::fromAxisAngleDegrees({0.0f, 1.0f, 0.0f}, angularSpeedDegrees) * delta;
        }
        if (ctx.input.isActionDown(engine::systems::InputAction::LookUp)) {
            delta = Quat::fromAxisAngleDegrees({1.0f, 0.0f, 0.0f}, -angularSpeedDegrees) * delta;
        }
        if (ctx.input.isActionDown(engine::systems::InputAction::LookDown)) {
            delta = Quat::fromAxisAngleDegrees({1.0f, 0.0f, 0.0f}, angularSpeedDegrees) * delta;
        }

        t.rotation = (delta * t.rotation).normalized();

        if (ctx.input.isMouseButtonDown(engine::systems::MouseButton::Right)) {
            constexpr float sensitivity = 0.1f;

            const float yaw = ctx.input.mouseDeltaX() * sensitivity;
            const float pitch = ctx.input.mouseDeltaY() * sensitivity;

            const auto mouseDelta =
                Quat::fromAxisAngleDegrees({0.0f, 1.0f, 0.0f}, yaw) *
                Quat::fromAxisAngleDegrees({1.0f, 0.0f, 0.0f}, pitch);

            t.rotation = (mouseDelta * t.rotation).normalized();
        }
        if (ctx.input.mouseWheelY() != 0.0f) {
            const auto cameraEntity = ctx.scene.getActiveCamera();
            if (cameraEntity.isValid() &&
                cameraEntity.scene == &ctx.scene &&
                ctx.scene.hasComponent<engine::components::CameraComponent,
                    engine::components::TransformComponent>(cameraEntity)) {
                auto& camera = ctx.scene.getComponent<engine::components::CameraComponent>(cameraEntity);
                camera.fovYDegrees -= ctx.input.mouseWheelY() * 2.0f;
                camera.fovYDegrees = std::clamp(camera.fovYDegrees, 20.0f, 100.0f);
            }
        }
        if (ctx.input.wasActionPressed(engine::systems::InputAction::Reset)) {
            t.reset();
        }

        if (ctx.input.wasActionPressed(engine::systems::InputAction::Quit)) {
            ctx.requestQuit();
        }
    }

    auto SMB::render(engine::core::EngineContext& ctx) -> void {
        const auto cameraEntity = ctx.scene.getActiveCamera();
        if (!cameraEntity.isValid() ||
            !ctx.scene.hasComponent<engine::components::TransformComponent, engine::components::CameraComponent>(cameraEntity)) {
            return;
        }

        const auto extent = ctx.renderer.backbufferExtent();
        const auto& cameraTransform = ctx.scene.getComponent<engine::components::TransformComponent>(cameraEntity);
        const auto& camera = ctx.scene.getComponent<engine::components::CameraComponent>(cameraEntity);

        const engine::graphics::SceneView view{
            .viewId = 0,
            .target = nullptr,
            .viewMatrix = engine::math::CameraMatrices::makeView(cameraTransform),
            .projectionMatrix = engine::math::CameraMatrices::makeProjection(camera, extent.width, extent.height),
            .viewport = {
                .width = extent.width,
                .height = extent.height
            },
            .clearFlags = toBgfxClearFlags(camera.clearFlags),
            .clearColor = camera.clearColor
        };

        ctx.renderer.renderScene(ctx.scene, ctx.resources, view);
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
