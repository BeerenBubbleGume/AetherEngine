//
// Created by drhaz on 14.06.2026.
//

#include "window/Window.hpp"


bool engine::Window::initWindow(std::string_view title, int width_, int height_) {
    width = width_;
    height = height_;
    window = SDL_CreateWindow(title.data(), width, height, SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);
    if (!window) {
        SDL_Log("Failed to create window: %s", SDL_GetError());
        return false;
    }
    isOpen.store(true, std::memory_order::release);
    return true;
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
    SDL_DestroyWindow(window);
}
