//
// Created by drhaz on 25.10.2026.
//

#ifndef AETHER_EDITOR_SIMULATIONCONTROLLER_HPP
#define AETHER_EDITOR_SIMULATIONCONTROLLER_HPP
#include <expected>
#include <memory>

#include "core/EngineConfig.hpp"
#include "scene/Scene.hpp"


namespace AetherEngine::core {
    struct EngineContext;
}

namespace AetherEditor::simulation {
    enum class SimulationState {
        Stopped,
        Running
    };
    class SimulationController {
    public:
        struct SimulationControllerDeleter {
            void operator()(SimulationController* controller) const {
                delete controller;
            }
        };
        using SimulationControllerPtr = std::unique_ptr<SimulationController, SimulationControllerDeleter>;
        SimulationController(const SimulationController&) = delete;
        SimulationController& operator=(const SimulationController&) = delete;
        SimulationController(SimulationController&&) = delete;
        SimulationController& operator=(SimulationController&&) = delete;

        [[nodiscard]] static auto createSimulationController() -> SimulationControllerPtr;
        [[nodiscard]] auto start(AetherEngine::core::EngineContext& ctx) -> std::expected<void, AetherEngine::core::EngineError>;
        [[nodiscard]] auto stop(AetherEngine::core::EngineContext& ctx) -> std::expected<void, AetherEngine::core::EngineError>;

        [[nodiscard]] auto isRunning() const noexcept -> bool;
    private:
        [[nodiscard]] static auto cloneScene(AetherEngine::scene::Scene& src) -> std::expected<AetherEngine::scene::Scene::ScenePtr, AetherEngine::core::EngineError>;
        SimulationController() = default;
        ~SimulationController() = default;
        AetherEngine::scene::Scene::ScenePtr m_editScene;
        SimulationState m_state = SimulationState::Stopped;
    };
} // simulation
// AetherEditor

#endif //AETHER_EDITOR_SIMULATIONCONTROLLER_HPP
