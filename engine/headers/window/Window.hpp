//
// Created by drhaz on 14.06.2026.
//

#ifndef SMB_WINDOW_HPP
#define SMB_WINDOW_HPP

#include <memory>
#include <string_view>
#include <SDL3/SDL.h>

namespace engine {
    class Window {
        struct WindowDeleter {
            void operator()(const Window* window) const {
                delete window;
            }
        };
    public:
        using Ptr = std::unique_ptr<Window, WindowDeleter>;

        [[nodiscard]] bool initWindow(std::string_view title, int width, int height);
        [[nodiscard]] SDL_Window* getSDLWindow() const;
        [[nodiscard]] int Width() const;
        [[nodiscard]] int Height() const;
        static Ptr createWindow();

    private:
        SDL_Window* window;
        Window();
        Window(const Window&) = delete;
        Window& operator=(const Window&) = delete;
        Window(Window&&) = delete;
        Window& operator=(Window&&) = delete;

        ~Window();
        std::atomic<bool> isOpen{false};
        int width{0};
        int height{0};
    };
}

#endif //SMB_WINDOW_HPP
