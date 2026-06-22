//
// Created by drhaz on 14.06.2026.
//

#include "Engine.hpp"

auto engine::Engine::initEngine() -> std::expected<void, EngineError> {
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return std::unexpected{EngineError{1, "Failed to initialize SDL"}}; 
    }
    sWindow = Window::createWindow();
    sInput = systems::SYInputSystem::createInputSystem();
    sRender = systems::SYRenderSystem::createRenderSystem();
    sResource = resources::RResourceManager::createResourceManager();

    if (!sWindow) {
        return std::unexpected{EngineError{2, "Failed to create window"}};
    }
    if (!sInput) {
        return std::unexpected{EngineError{2, "Failed to create input system"}};
    }
    if (!sRender) {
        return std::unexpected{EngineError{2, "Failed to create render system"}};
    }
    if (!sResource) {
        return std::unexpected{EngineError{2, "Failed to create resource manager"}};
    }
    auto resultInitWindow = sWindow->initWindow("SMB Engine", 1440, 1080);
    if (!resultInitWindow) {
        std::cerr << "Failed to init window: " << resultInitWindow.error().message << std::endl;
        return std::unexpected{EngineError{3, "Failed to init window"}};
    }
    auto resultInitRenderer = sRender->init(*sWindow->getSDLWindow());
    if (!resultInitRenderer) {
        std::cerr << "Failed to init renderer: " << resultInitRenderer.error().message << std::endl;
        return std::unexpected{EngineError{3, "Failed to init renderer"}};
    }
    return std::expected<void, EngineError>{};
}

auto engine::Engine::run() -> std::expected<void, EngineError> {
    isRunning = true;
    TimePoint lastTime = Clock::now();

    auto program = sResource->loadProgram("basic",
        "D:/smb/assets/shaders/bin/basic_vs.bin",
        "D:/smb/assets/shaders/bin/basic_fs.bin");

    if (!program.isValid()) {
        return std::unexpected{EngineError{3, "Failed to load program"}};
    }
    auto mesh = sResource->createTriangleMesh("triangle");
    if (!mesh.isValid()) {
        std::cerr << "Failed to create mesh" << std::endl;
        return std::unexpected{EngineError{3, "Failed to create mesh"}};
    }
    auto scene = scene::SCScene::createScene();
    auto camera = scene::SCCamera();
    scene->addCamera(std::move(camera));
    scene->addObject({
        .transform = graphics::GTransform{
            .position = {0.0f, 0.0f, 0.0f},
            .rotation = {0.0f, 0.0f, 0.0f},
            .scale = {1.0f, 1.0f, 1.0f}
        },
        .mesh = mesh,
        .program = program
    });

    while (isRunning) {
        TimePoint now = Clock::now();
        auto delta = std::chrono::duration<float>(now - lastTime).count();
        lastTime = now;

        accumulator += delta;
        processEvents();
        while (accumulator >= FIXED_DT) {
            accumulator -= FIXED_DT;
        }
        sRender->render(*scene, *sResource);
        update(delta, *scene, 0);
    }
    return std::expected<void, EngineError>{};
}

engine::Engine::EnginePtr engine::Engine::createEngine() {
    return EnginePtr(new Engine, EngineDeleter{});
}

engine::Engine::~Engine() {
    if (sWindow) {
        sWindow.reset();
    }
    if (sInput) {
        sInput.reset();
    }
    if (sRender) {
        sRender.reset();
    }
    if (sResource) {
        sResource.reset();
    }
    bgfx::shutdown();
    SDL_Quit();
}

auto engine::Engine::processEvents() -> void {
    SDL_Event event;

    while (SDL_PollEvent(&event))
    {
        if (event.type == SDL_EVENT_QUIT){
            isRunning = false;
        }

        sInput->processEvents(event);
    }
}

auto engine::Engine::update(float delta, scene::SCScene &scene, scene::SObjectId objectId) const -> void {
    sInput->update(delta);

    auto& object = scene.getObject(objectId);
    auto& t = object.transform;

    const float speed = 1.0f * delta;
    const float rotSpeed = 90.0f * delta;

    if (sInput->isKeyPressed(SDL_SCANCODE_A)) t.position.x += speed;
    if (sInput->isKeyPressed(SDL_SCANCODE_D)) t.position.x -= speed;
    if (sInput->isKeyPressed(SDL_SCANCODE_W)) t.position.y += speed;
    if (sInput->isKeyPressed(SDL_SCANCODE_S)) t.position.y -= speed;

    if (sInput->isKeyPressed(SDL_SCANCODE_LEFT))  t.rotation.y -= rotSpeed;
    if (sInput->isKeyPressed(SDL_SCANCODE_RIGHT)) t.rotation.y += rotSpeed;
    if (sInput->isKeyPressed(SDL_SCANCODE_UP))    t.rotation.x -= rotSpeed;
    if (sInput->isKeyPressed(SDL_SCANCODE_DOWN))  t.rotation.x += rotSpeed;

    if (sInput->isKeyPressed(SDL_SCANCODE_R)) t.reset();
}
