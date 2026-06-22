//
// Created by drhaz on 22.06.2026.
//

#include "core/Smb.hpp"

namespace smb {
    auto SMB::init(engine::core::EngineContext &ctx) -> std::expected<void, engine::core::EngineError> {
        auto program = ctx.resources.loadProgram("basic", (ctx.paths.shadersRoot/"bin/basic_vs.bin").string(), (ctx.paths.shadersRoot/"bin/basic_fs.bin").string());

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
            .rotation = {0.0f, 0.0f, 0.0f},
            .scale = { 0.0f, 1.0f, 0.0f }
        });
        ctx.scene.addCamera(camera);

        playerObjectId = ctx.scene.addObject({
            .transform = engine::graphics::GTransform{
                .position = {0.0f, 0.0f, 0.0f},
                .rotation = {0.0f, 0.0f, 0.0f},
                .scale = {1.0f, 1.0f, 1.0f}
            },
            .mesh = mesh,
            .program = program
        });

        return {};
    }

    auto SMB::update(float dt, engine::core::EngineContext &ctx) -> void {
        if (!playerObjectId) {
            return;
        }
        auto& object = ctx.scene.getObject(*playerObjectId);
        auto& t = object.transform;

        const float speed = 1.0f * dt;
        const float rotSpeed = 90.0f * dt;

        if (ctx.input.isKeyPressed(SDL_SCANCODE_A)) t.position.x += speed;
        if (ctx.input.isKeyPressed(SDL_SCANCODE_D)) t.position.x -= speed;
        if (ctx.input.isKeyPressed(SDL_SCANCODE_W)) t.position.y += speed;
        if (ctx.input.isKeyPressed(SDL_SCANCODE_S)) t.position.y -= speed;

        if (ctx.input.isKeyPressed(SDL_SCANCODE_LEFT))  t.rotation.y -= rotSpeed;
        if (ctx.input.isKeyPressed(SDL_SCANCODE_RIGHT)) t.rotation.y += rotSpeed;
        if (ctx.input.isKeyPressed(SDL_SCANCODE_UP))    t.rotation.x -= rotSpeed;
        if (ctx.input.isKeyPressed(SDL_SCANCODE_DOWN))  t.rotation.x += rotSpeed;

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
