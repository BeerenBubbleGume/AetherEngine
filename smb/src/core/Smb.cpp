//
// Created by drhaz on 22.06.2026.
//

#include "core/Smb.hpp"

#include <algorithm>

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
        using AetherEngine::math::Quat;
        using AetherEngine::math::Vec3;

        auto toBgfxClearFlags(uint8_t cameraFlags) -> uint16_t {
            uint16_t flags = 0;
            if ((cameraFlags & AetherEngine::components::ClearColor) != 0) {
                flags |= BGFX_CLEAR_COLOR;
            }
            if ((cameraFlags & AetherEngine::components::ClearDepth) != 0) {
                flags |= BGFX_CLEAR_DEPTH;
            }
            if ((cameraFlags & AetherEngine::components::ClearStencil) != 0) {
                flags |= BGFX_CLEAR_STENCIL;
            }
            return flags;
        }
    }

    auto SMB::init(AetherEngine::core::EngineContext &ctx) -> std::expected<void, AetherEngine::core::EngineError> {
        ctx.input.bindKey(AetherEngine::systems::InputAction::MoveLeft, AetherEngine::systems::Key::A);
        ctx.input.bindKey(AetherEngine::systems::InputAction::MoveRight, AetherEngine::systems::Key::D);
        ctx.input.bindKey(AetherEngine::systems::InputAction::MoveForward, AetherEngine::systems::Key::W);
        ctx.input.bindKey(AetherEngine::systems::InputAction::MoveBackward, AetherEngine::systems::Key::S);
        ctx.input.bindKey(AetherEngine::systems::InputAction::RotateLeft, AetherEngine::systems::Key::Left);
        ctx.input.bindKey(AetherEngine::systems::InputAction::RotateRight, AetherEngine::systems::Key::Right);
        ctx.input.bindKey(AetherEngine::systems::InputAction::LookUp, AetherEngine::systems::Key::Up);
        ctx.input.bindKey(AetherEngine::systems::InputAction::LookDown, AetherEngine::systems::Key::Down);
        ctx.input.bindKey(AetherEngine::systems::InputAction::Reset, AetherEngine::systems::Key::R);
        ctx.input.bindKey(AetherEngine::systems::InputAction::Quit, AetherEngine::systems::Key::Escape);
        if (!ctx.scene.isEmpty()) {
            auto player = ctx.scene.findEntityById("player");
            if (!player) {
                return std::unexpected(AetherEngine::core::EngineError{1, "Failed to find player entity"});
            }
            if (!ctx.scene.hasComponent<AetherEngine::components::TransformComponent>(*player)) {
                return std::unexpected(AetherEngine::core::EngineError{1, "Player entity has no Transform component"});
            }
            playerEntity = player;
            return {};
        }

        const auto meshAsset = ctx.assets.reference<AetherEngine::assets::RuntimeMeshAsset>(
            "asset://meshes/bunny"
        );
        if (!meshAsset) {
            return std::unexpected(AetherEngine::core::EngineError{1, meshAsset.error().message});
        }
        const auto materialAsset = ctx.assets.reference<AetherEngine::assets::RuntimeMaterialAsset>(
            "asset://materials/brick"
        );
        if (!materialAsset) {
            return std::unexpected(AetherEngine::core::EngineError{1, materialAsset.error().message});
        }
        const auto mesh = ctx.assets.load(*meshAsset);
        if (!mesh) {
            return std::unexpected(AetherEngine::core::EngineError{1, mesh.error().message});
        }
        const auto material = ctx.assets.load(*materialAsset);
        if (!material) {
            return std::unexpected(AetherEngine::core::EngineError{1, material.error().message});
        }

        auto camera = ctx.scene.createEntity();
        ctx.scene.addComponent<AetherEngine::components::IdentityComponent>(camera, AetherEngine::components::IdentityComponent{
            .id = "camera",
            .name = "Camera"
        });

        ctx.scene.addComponent<AetherEngine::components::TransformComponent>(camera, AetherEngine::components::TransformComponent{
            .transform = {
                .position = {0.0f, 0.0f, 5.0f},
                .rotation = Quat::identity(),
                .scale = Vec3::one()
            }
        });

        ctx.scene.addComponent<AetherEngine::components::CameraComponent>(camera, AetherEngine::components::CameraComponent{
            .fovYDegrees = 60.0f,
            .nearPlane = 0.1f,
            .farPlane = 100.0f
        });

        ctx.scene.setActiveCamera(camera);

        auto player = ctx.scene.createEntity();
        ctx.scene.addComponent<AetherEngine::components::IdentityComponent>(player, AetherEngine::components::IdentityComponent{
            .id = "player",
            .name = "Player"
        });
        ctx.scene.addComponent<AetherEngine::components::TransformComponent>(player, AetherEngine::components::TransformComponent {
            .transform = AetherEngine::graphics::Transform{
                .position = {0.0f, 0.0f, 0.0f},
                .rotation = Quat::identity(),
                .scale = {1.0f, 1.0f, 1.0f}
            }
        });
        ctx.scene.addComponent<AetherEngine::components::MeshComponent>(player, AetherEngine::components::MeshComponent {
            .asset = *meshAsset,
            .runtime = *mesh
        });
        ctx.scene.addComponent<AetherEngine::components::MaterialComponent>(player, AetherEngine::components::MaterialComponent {
            .asset = *materialAsset,
            .runtime = *material,
            .baseColor = {0.0f, 0.0f, 1.0f, 1.0f}
        });

        auto secondBunny = ctx.scene.createEntity();
        ctx.scene.addComponent<AetherEngine::components::IdentityComponent>(secondBunny, AetherEngine::components::IdentityComponent{
            .id = "second_bunny",
            .name = "Second Bunny"
        });
        ctx.scene.addComponent<AetherEngine::components::TransformComponent>(secondBunny, AetherEngine::components::TransformComponent {
            .transform = AetherEngine::graphics::Transform{
                .position = {-5.0f, -5.0f, -5.0f},
                .rotation = Quat::fromAxisAngleDegrees({0.0f, 1.0f, 0.0f}, 30.0f) *
                            Quat::fromAxisAngleDegrees({1.0f, 0.0f, 0.0f}, 40.0f),
                .scale = {2.0f, 2.0f, 2.0f}
            },
        });
        ctx.scene.addComponent<AetherEngine::components::MeshComponent>(secondBunny, AetherEngine::components::MeshComponent {
            .asset = *meshAsset,
            .runtime = *mesh
        });
        ctx.scene.addComponent<AetherEngine::components::MaterialComponent>(secondBunny, AetherEngine::components::MaterialComponent {
            .asset = *materialAsset,
            .runtime = *material,
            .baseColor = {1.0f, 0.0f, 0.0f, 1.0f}
        });
        ctx.scene.addComponent<AetherEngine::components::ColliderComponent>(secondBunny, AetherEngine::components::ColliderComponent {});
        ctx.scene.addComponent<AetherEngine::components::RigidbodyComponent>(secondBunny, AetherEngine::components::RigidbodyComponent {});
        playerEntity = player;
        auto serializeResult = ctx.sceneSerializer.serializeScene(ctx.scene);
        if (!serializeResult) {
            return std::unexpected(AetherEngine::core::EngineError{1, serializeResult.error().message});
        }
        return {};
    }

    auto SMB::update(float dt, AetherEngine::core::EngineContext &ctx) -> void {
        if (!playerEntity || !playerEntity->isValid() || playerEntity->scene != &ctx.scene ||
            !ctx.scene.hasComponent<AetherEngine::components::TransformComponent>(*playerEntity)) {
            return;
        }
        auto& object = ctx.scene.getComponent<AetherEngine::components::TransformComponent>(*playerEntity);

        auto& t = object.transform;

        const float speed = 1.0f * dt;
        const float angularSpeedDegrees = 90.0f * dt;

        if (ctx.input.isActionDown(AetherEngine::systems::InputAction::MoveLeft)) t.position.x += speed;
        if (ctx.input.isActionDown(AetherEngine::systems::InputAction::MoveRight)) t.position.x -= speed;
        if (ctx.input.isActionDown(AetherEngine::systems::InputAction::MoveForward)) t.position.y += speed;
        if (ctx.input.isActionDown(AetherEngine::systems::InputAction::MoveBackward)) t.position.y -= speed;

        Quat delta = Quat::identity();

        if (ctx.input.isActionDown(AetherEngine::systems::InputAction::RotateLeft)) {
            delta = Quat::fromAxisAngleDegrees({0.0f, 1.0f, 0.0f}, -angularSpeedDegrees) * delta;
        }
        if (ctx.input.isActionDown(AetherEngine::systems::InputAction::RotateRight)) {
            delta = Quat::fromAxisAngleDegrees({0.0f, 1.0f, 0.0f}, angularSpeedDegrees) * delta;
        }
        if (ctx.input.isActionDown(AetherEngine::systems::InputAction::LookUp)) {
            delta = Quat::fromAxisAngleDegrees({1.0f, 0.0f, 0.0f}, -angularSpeedDegrees) * delta;
        }
        if (ctx.input.isActionDown(AetherEngine::systems::InputAction::LookDown)) {
            delta = Quat::fromAxisAngleDegrees({1.0f, 0.0f, 0.0f}, angularSpeedDegrees) * delta;
        }

        t.rotation = (delta * t.rotation).normalized();

        if (ctx.input.isMouseButtonDown(AetherEngine::systems::MouseButton::Right)) {
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
                ctx.scene.hasComponent<AetherEngine::components::CameraComponent,
                    AetherEngine::components::TransformComponent>(cameraEntity)) {
                auto& camera = ctx.scene.getComponent<AetherEngine::components::CameraComponent>(cameraEntity);
                camera.fovYDegrees -= ctx.input.mouseWheelY() * 2.0f;
                camera.fovYDegrees = std::clamp(camera.fovYDegrees, 20.0f, 100.0f);
            }
        }
        if (ctx.input.wasActionPressed(AetherEngine::systems::InputAction::Reset)) {
            t.reset();
        }

        if (ctx.input.wasActionPressed(AetherEngine::systems::InputAction::Quit)) {
            ctx.requestQuit();
        }
    }

    auto SMB::render(AetherEngine::core::EngineContext& ctx) -> void {
        const auto cameraEntity = ctx.scene.getActiveCamera();
        if (!cameraEntity.isValid() ||
            !ctx.scene.hasComponent<AetherEngine::components::TransformComponent, AetherEngine::components::CameraComponent>(cameraEntity)) {
            return;
        }

        const auto extent = ctx.renderer.backbufferExtent();
        const auto& cameraTransform = ctx.scene.getComponent<AetherEngine::components::TransformComponent>(cameraEntity);
        const auto& camera = ctx.scene.getComponent<AetherEngine::components::CameraComponent>(cameraEntity);

        const AetherEngine::graphics::SceneView view{
            .viewId = 0,
            .target = nullptr,
            .viewMatrix = AetherEngine::math::CameraMatrices::makeView(cameraTransform),
            .projectionMatrix = AetherEngine::math::CameraMatrices::makeProjection(camera, extent.width, extent.height),
            .viewport = {
                .width = extent.width,
                .height = extent.height
            },
            .clearFlags = toBgfxClearFlags(camera.clearFlags),
            .clearColor = camera.clearColor
        };

        ctx.renderer.renderScene(ctx.scene, ctx.assets, view);
    }

    auto SMB::run() -> void {
        // This function is intentionally left empty, as the main loop is handled by the AetherEngine.
    }

    auto SMB::createSMB() -> SMBPtr {
        return SMBPtr(new SMB(), SMBDeleter{});
    }


    SMB::~SMB() {
    }
} // smb
