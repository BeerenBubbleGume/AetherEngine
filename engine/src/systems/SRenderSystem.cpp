//
// Created by drhaz on 19.06.2026.
//

#include "systems/SRenderSystem.hpp"

struct PosColorVertex {
    float x, y, z;
    uint32_t abgr;
};

static const PosColorVertex s_triangleVertices[3] = {
    {-0.5f, -0.5f, 0.0f, 0xff0000ff}, // красный
    { 0.5f, -0.5f, 0.0f, 0xff00ff00}, // зелёный
    { 0.0f,  0.5f, 0.0f, 0xffff0000}  // синий
};

static const uint16_t s_triangleIndices[3] = { 0, 1, 2 };
static bgfx::VertexLayout s_vertexLayout;

engine::systems::SRenderSystem::SRenderSystemPtr engine::systems::SRenderSystem::createRenderSystem() {
    return SRenderSystemPtr(new SRenderSystem());
}

void engine::systems::SRenderSystem::render() {
    int width, height;
    SDL_GetWindowSize(rWindow, &width, &height);
    bgfx::setViewRect(0, 0, 0, uint16_t(width), uint16_t(height));

    bgfx::touch(0); // на всякий случай

    // === Рисуем треугольник ===
    bgfx::setVertexBuffer(0, m_vbh);
    bgfx::setIndexBuffer(m_ibh);

    if (bgfx::isValid(m_program)) {
        bgfx::submit(0, m_program);
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
        s_vertexLayout.begin()
            .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
            .add(bgfx::Attrib::Color0,   4, bgfx::AttribType::Uint8, true) // normalized
            .end();

        // === Создаём буферы ===
        m_vbh = bgfx::createVertexBuffer(
            bgfx::makeRef(s_triangleVertices, sizeof(s_triangleVertices)),
            s_vertexLayout
        );

        m_ibh = bgfx::createIndexBuffer(
            bgfx::makeRef(s_triangleIndices, sizeof(s_triangleIndices))
        );

        // === Шейдеры ===
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
    if (bgfx::isValid(m_vbh)) bgfx::destroy(m_vbh);
    if (bgfx::isValid(m_ibh)) bgfx::destroy(m_ibh);
    if (bgfx::isValid(m_program)) bgfx::destroy(m_program);
    bgfx::shutdown();
}
