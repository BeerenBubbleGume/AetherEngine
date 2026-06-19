//
// Created by drhaz on 14.06.2026.
//

#ifndef SMB_ENGINE_HPP
#define SMB_ENGINE_HPP

#include <chrono>
#include <expected>
#include <chrono>
#include <iostream>
#include <ostream>
#include <ratio>
#include "systems/SInputSystem.hpp"
#include "window/Window.hpp"

namespace engine {
    class EngineError {
        public:
        int code;
        const char* message;
    };
    class Engine {
    public:
        Engine(const Engine&) = delete;
        Engine& operator=(const Engine&) = delete;
        Engine(Engine&&) = delete;
        Engine& operator=(Engine&&) = delete;

        struct EngineDeleter {
            void operator()(const Engine* engine) const {
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
        ~Engine();

        bool isRunning{false};

        [[nodiscard]] auto render() -> std::expected<void, EngineError>;

        auto processEvents() -> void;
        auto update(float delta) -> void;


        using WindowPtr = std::unique_ptr<Window, Window::WindowDeleter>;
        using SInputPtr = std::unique_ptr<systems::SInputSystem, systems::SInputSystem::SInputDeleter>;
        using Clock = std::chrono::steady_clock;
        using Duration = std::chrono::duration<float>;
        using TimePoint = Clock::time_point;
        WindowPtr sWindow;
        SInputPtr sInput;

        float accumulator = 0.0;
        const float FIXED_DT = 1.0 / 120.0;

    };
}



#endif //SMB_ENGINE_HPP
