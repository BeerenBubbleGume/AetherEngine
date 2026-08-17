//
// Created by drhaz on 14.06.2026.
//

#include "window/Window.hpp"


auto AetherEngine::Window::initWindow(std::string_view title, int width_, int height_) -> std::expected<void, WindowError> {
    width = width_;
    height = height_;
    window = SDL_CreateWindow(title.data(), width, height, SDL_WINDOW_RESIZABLE | SDL_WINDOW_INPUT_FOCUS);
    if (!window) {
        SDL_Log("Failed to create window: %s", SDL_GetError());
        return std::unexpected{WindowError{1, "Failed to create window"}};
    }
    isOpen.store(true, std::memory_order::release);
    return std::expected<void, WindowError>{};
}

auto AetherEngine::Window::shutdown() -> std::expected<void, WindowError> {
    if (window) {
        try {
            SDL_DestroyWindow(window);
            window = nullptr;
            isOpen.store(false, std::memory_order::release);
            return std::expected<void, WindowError>{};
        } catch (const std::exception& e) {
            return std::unexpected{WindowError{2, e.what()}};
        }
    }
    return std::expected<void, WindowError>{};
}

AetherEngine::Window::Ptr AetherEngine::Window::createWindow() {
    return Ptr(new Window, WindowDeleter{});
}

SDL_Window * AetherEngine::Window::getSDLWindow() const {
    return window;
}

int AetherEngine::Window::Width() const {
    return width;
}

int AetherEngine::Window::Height() const {
    return height;
}

AetherEngine::Window::Window() : window(nullptr) {
}

AetherEngine::Window::~Window() {
    if (window) {
        SDL_DestroyWindow(window);
        window = nullptr;
    }
}
