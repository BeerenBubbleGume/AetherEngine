//
// Created by drhaz on 22.06.2026.
//

#include "core/Smb.hpp"

#include "components/IMeshComponent.hpp"
#include "components/ITransformComponent.hpp"

namespace smb {
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
        auto camera = engine::scene::SCCamera();
        camera.setTransform({
            .position = { 0.0f, 0.0f, 5.0f },
            .rotation = glm::quat{1.0f, 0.0f, 0.0f, 0.0f},
            .scale = { 0.0f, 1.0f, 0.0f }
        });
        ctx.scene.addCamera(camera);

        auto player = ctx.scene.createEntity();
        ctx.scene.addComponent<engine::components::ITransformComponent>(player, engine::components::ITransformComponent {
            .transform = engine::graphics::GTransform{
                .position = {0.0f, 0.0f, 0.0f},
                .rotation = glm::quat{1.0f, 0.0f, 0.0f, 0.0f},
                .scale = {1.0f, 1.0f, 1.0f}
            }
        });
        ctx.scene.addComponent<engine::components::IMeshComponent>(player, engine::components::IMeshComponent {
            .mesh = mesh,
            .program = program
        });

        auto secondBunny = ctx.scene.createEntity();
        ctx.scene.addComponent<engine::components::ITransformComponent>(secondBunny, engine::components::ITransformComponent {
            .transform = engine::graphics::GTransform{
                .position = {-5.0f, -5.0f, -5.0f},
                .rotation = glm::quat{1.0f, 30.0f, 40.0f, 0.0f},
                .scale = {2.0f, 2.0f, 2.0f}
            },
        });
        ctx.scene.addComponent<engine::components::IMeshComponent>(secondBunny, engine::components::IMeshComponent {
            .mesh = mesh,
            .program = program
        });

        auto triangle = ctx.scene.createEntity();
        ctx.scene.addComponent<engine::components::ITransformComponent>(triangle, engine::components::ITransformComponent {
            .transform = engine::graphics::GTransform{
                .position = {5.0f, -5.0f, -5.0f},
                .rotation = glm::quat{1.0f, 10.0f, 0.0f, 5.0f},
                .scale = {2.0f, 2.0f, 2.0f}
            },
        });
        ctx.scene.addComponent<engine::components::IMeshComponent>(triangle, engine::components::IMeshComponent {
            .mesh = ctx.resources.createTriangleMesh("triangle"),
            .program = program
        });
        playerEntity = player;
        return {};
    }

    auto SMB::update(float dt, engine::core::EngineContext &ctx) -> void {
        if (!playerEntity) {
            return;
        }
        //auto& object = ctx.scene.getObject(*playerObjectId);
        auto& object = ctx.scene.getComponent<engine::components::ITransformComponent>(*playerEntity);

        auto& t = object.transform;

        const float speed = 1.0f * dt;
        const float angularSpeed = glm::radians(90.0f) * dt;

        if (ctx.input.isKeyPressed(SDL_SCANCODE_A)) t.position.x += speed;
        if (ctx.input.isKeyPressed(SDL_SCANCODE_D)) t.position.x -= speed;
        if (ctx.input.isKeyPressed(SDL_SCANCODE_W)) t.position.y += speed;
        if (ctx.input.isKeyPressed(SDL_SCANCODE_S)) t.position.y -= speed;

        glm::quat delta = glm::quat{1.0f, 0.0f, 0.0f, 0.0f};

        if (ctx.input.isKeyPressed(SDL_SCANCODE_LEFT)) {
            delta = glm::angleAxis(-angularSpeed, glm::vec3{0.0f, 1.0f, 0.0f}) * delta;
        }
        if (ctx.input.isKeyPressed(SDL_SCANCODE_RIGHT)) {
            delta = glm::angleAxis(angularSpeed, glm::vec3{0.0f, 1.0f, 0.0f}) * delta;
        }
        if (ctx.input.isKeyPressed(SDL_SCANCODE_UP)) {
            delta = glm::angleAxis(-angularSpeed, glm::vec3{1.0f, 0.0f, 0.0f}) * delta;
        }
        if (ctx.input.isKeyPressed(SDL_SCANCODE_DOWN)) {
            delta = glm::angleAxis(angularSpeed, glm::vec3{1.0f, 0.0f, 0.0f}) * delta;
        }

        t.rotation = glm::normalize(delta * t.rotation);

        if (ctx.input.isKeyPressed(SDL_SCANCODE_R)) t.reset();
        if (ctx.input.isKeyPressed(SDL_SCANCODE_Q)) ctx.requestQuit();
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
