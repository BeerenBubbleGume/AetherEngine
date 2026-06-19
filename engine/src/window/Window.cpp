//
// Created by drhaz on 14.06.2026.
//

#include "window/Window.hpp"

#include "Engine.hpp"


auto engine::Window::initWindow(std::string_view title, int width_, int height_) -> std::expected<void, WindowError> {
    width = width_;
    height = height_;
    window = SDL_CreateWindow(title.data(), width, height, SDL_WINDOW_RESIZABLE);
    if (!window) {
        SDL_Log("Failed to create window: %s", SDL_GetError());
        return std::unexpected{WindowError{1, "Failed to create window"}};
    }
    isOpen.store(true, std::memory_order::release);
    return std::expected<void, WindowError>{};
}

auto engine::Window::shutdown() -> std::expected<void, WindowError> {
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

engine::Window::Ptr engine::Window::createWindow() {
    return std::unique_ptr<Window, WindowDeleter>(new Window, WindowDeleter{});
}

SDL_Window * engine::Window::getSDLWindow() const {
    return window;
}

int engine::Window::Width() const {
    return width;
}

int engine::Window::Height() const {
    return height;
}

engine::Window::Window() : window(nullptr) {
}

engine::Window::~Window() {
    if (window) {
        SDL_DestroyWindow(window);
    }
}
