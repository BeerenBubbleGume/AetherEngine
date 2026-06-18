//
// Created by drhaz on 14.06.2026.
//

#ifndef SMB_WINDOW_HPP
#define SMB_WINDOW_HPP

#include <memory>
#include <string_view>
#include <SDL3/SDL.h>
#include <expected>

namespace engine {
    class WindowError {
        public:
        int code;
        const char* message;
    };
    class Window {
    public:
        struct WindowDeleter {
            void operator()(const Window* window) const {
                delete window;
            }
        };
        using Ptr = std::unique_ptr<Window, WindowDeleter>;

        [[nodiscard]] auto initWindow(std::string_view title, int width, int height) -> std::expected<void, WindowError>;
        [[nodiscard]] auto shutdown() -> std::expected<void, WindowError>;
        [[nodiscard]] auto getSDLWindow() const -> SDL_Window*;
        [[nodiscard]] auto Width() const -> int;
        [[nodiscard]] auto Height() const -> int;
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
