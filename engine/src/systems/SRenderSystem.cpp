//
// Created by drhaz on 19.06.2026.
//

#include "systems/SRenderSystem.hpp"

engine::systems::SRenderSystem::SRenderSystemPtr engine::systems::SRenderSystem::createRenderSystem() {
    return SRenderSystemPtr(new SRenderSystem());
}

void engine::systems::SRenderSystem::render() const {
    int width, height;
    SDL_GetWindowSize(rWindow, &width, &height);
    bgfx::setViewRect(0, 0, 0, uint16_t(width), uint16_t(height));

    bgfx::touch(0); // на всякий случай

    for (const auto& mesh : m_meshes) {
        if (mesh && mesh->isValid()) {
            mesh->submit(m_program);
        }
    }

    bgfx::frame();
}

auto engine::systems::SRenderSystem::init(SDL_Window &window) -> std::expected<void, SRenderSystemError> {
    try {
        rWindow = &window;
        SDL_PropertiesID props = SDL_GetWindowProperties(rWindow);
        bgfx::PlatformData pd;
#if defined(SDL_PLATFORM_WIN32)
        pd.nwh = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);
        pd.ndt = nullptr;

#elif defined(SDL_PLATFORM_MACOS)
        pd.nwh = SDL_GetProperty(props, SDL_PROP_WINDOW_COCOA_WINDOW_POINTER, nullptr);
        pd.ndt = nullptr;

#elif defined(SDL_PLATFORM_LINUX)
        const char* driver = SDL_GetCurrentVideoDriver();
        if (SDL_strcmp(driver, "x11") == 0) {
            pd.ndt = SDL_GetProperty(props, SDL_PROP_WINDOW_X11_DISPLAY_POINTER, nullptr);
            pd.nwh = (void*)(uintptr_t)SDL_GetNumberProperty(props, SDL_PROP_WINDOW_X11_WINDOW_NUMBER, 0);
        } else if (SDL_strcmp(driver, "wayland") == 0) {
            pd.ndt = SDL_GetProperty(props, SDL_PROP_WINDOW_WAYLAND_DISPLAY_POINTER, nullptr);
            pd.nwh = SDL_GetProperty(props, SDL_PROP_WINDOW_WAYLAND_SURFACE_POINTER, nullptr);
        }
#endif
        bgfx::Init init;

        bgfx::setPlatformData(pd);
        init.platformData = pd;
        init.type = bgfx::RendererType::Count; // авто-выбор

        int width, height;
        SDL_GetWindowSize(rWindow, &width, &height);
        init.resolution.width  = static_cast<uint32_t>(width);
        init.resolution.height = static_cast<uint32_t>(height);
        init.resolution.reset  = BGFX_RESET_VSYNC;

        if (!bgfx::init(init)) {
            return std::unexpected{SRenderSystemError{1, "Failed to initialize bgfx"}};
        }

        bgfx::ShaderHandle vsh = loadShader("D:/smb/assets/shaders/bin/basic_vs.bin");
        bgfx::ShaderHandle fsh = loadShader("D:/smb/assets/shaders/bin/basic_fs.bin");

        m_program = bgfx::createProgram(vsh, fsh, true);

        bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x303030ff, 1.0f, 0);
        bgfx::setViewRect(0, 0, 0, uint16_t(width), uint16_t(height));

        return {};
    } catch (const std::exception& e) {
        return std::unexpected{SRenderSystemError{1, e.what()}};
    }
}

auto engine::systems::SRenderSystem::addMesh(graphics::GMesh::GMeshPtr mesh) -> std::expected<void, SRenderSystemError> {
    if (!mesh) {
        return std::unexpected{SRenderSystemError{1, "Mesh is nullptr"}};
    }
    try {
        m_meshes.push_back(std::move(mesh));
    } catch (const std::exception& e) {
        return std::unexpected{SRenderSystemError{1, e.what()}};
    }
    return {};
}

bgfx::ShaderHandle engine::systems::SRenderSystem::loadShader(const char *_filename) {
    std::ifstream file(_filename, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        return BGFX_INVALID_HANDLE;
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    const bgfx::Memory* mem = bgfx::alloc(static_cast<uint32_t>(size + 1));
    if (file.read(reinterpret_cast<char*>(mem->data), size)) {
        mem->data[mem->size - 1] = '\0';
        return bgfx::createShader(mem);
    }

    return BGFX_INVALID_HANDLE;
}

engine::systems::SRenderSystem::~SRenderSystem() {
    shutdown();
}

void engine::systems::SRenderSystem::shutdown() const {
    if (bgfx::isValid(m_program)) bgfx::destroy(m_program);
    bgfx::shutdown();
}
