//
// Created by drhaz on 14.06.2026.
//

#ifndef SMB_ENGINE_HPP
#define SMB_ENGINE_HPP

#include <expected>
#include "window/Window.hpp"

namespace engine {
    class EngineError {
        public:
        int code;
        const char* message;
    };
    class Engine {
    public:
        struct EngineDeleter {
            void operator()(Engine* engine) const {
                delete engine;
            }
        };
        using EnginePtr = std::unique_ptr<Engine, EngineDeleter>;

        [[nodiscard]] auto initEngine() -> std::expected<void, EngineError>;
        [[nodiscard]] auto run() -> std::expected<void, EngineError>;
        [[nodiscard]] auto shutdown() const -> std::expected<void, EngineError>;
        static EnginePtr createEngine();
    private:
        Engine() = default;
        Engine(const Engine&) = delete;
        Engine& operator=(const Engine&) = delete;
        ~Engine();

        bool isRunning{false};

        [[nodiscard]] auto processEvents() -> std::expected<void, EngineError>;
        [[nodiscard]] auto render() -> std::expected<void, EngineError>;
        [[nodiscard]] auto update(float delta) -> std::expected<void, EngineError>;


        using WindowPtr = std::unique_ptr<Window, Window::WindowDeleter>;
        WindowPtr window;


    };
}



#endif //SMB_ENGINE_HPP
