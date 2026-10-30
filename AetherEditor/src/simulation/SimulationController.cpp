//
// Created by drhaz on 25.10.2026.
//

#include "simulation/SimulationController.hpp"

#include <exception>
#include <string>
#include <utility>

#include "commands/EntitySnapshot.hpp"
#include "core/EngineContext.hpp"

namespace AetherEditor::simulation {
    auto SimulationController::createSimulationController() -> SimulationControllerPtr {
        return SimulationControllerPtr(new SimulationController(), SimulationControllerDeleter{});
    }

    auto SimulationController::start(AetherEngine::core::EngineContext &ctx) -> std::expected<void, AetherEngine::core::EngineError> {
        if (isRunning()) {
            return std::unexpected(AetherEngine::core::EngineError{2, "Simulation is already running"});
        }
        if (!ctx.getScene || !ctx.exchangeScene) {
            return std::unexpected(AetherEngine::core::EngineError{2, "Simulation requires scene access and exchange callbacks"});
        }
        try {
            auto runtimeScene = cloneScene(ctx.getScene());
            if (!runtimeScene) {
                return std::unexpected(runtimeScene.error());
            }
            auto result = ctx.exchangeScene(*runtimeScene);
            if (!result) {
                return std::unexpected(result.error());
            }
            m_editScene = std::move(*runtimeScene);
            m_state = SimulationState::Running;
            return {};
        } catch (const std::exception& e) {
            return std::unexpected(AetherEngine::core::EngineError{2, std::string{"Failed to start simulation: "} + e.what()});
        } catch (...) {
            return std::unexpected(AetherEngine::core::EngineError{2, "Failed to start simulation: unknown exception"});
        }
    }

    auto SimulationController::stop(AetherEngine::core::EngineContext &ctx) -> std::expected<void, AetherEngine::core::EngineError> {
        if (!isRunning()) {
            return std::unexpected(AetherEngine::core::EngineError{2, "Simulation is already stopped"});
        }
        if (!ctx.exchangeScene) {
            return std::unexpected(AetherEngine::core::EngineError{2, "Simulation requires a scene exchange callback"});
        }
        try {
            auto result = ctx.exchangeScene(m_editScene);
            if (!result) {
                return std::unexpected(result.error());
            }
            m_editScene.reset();
            m_state = SimulationState::Stopped;
            return {};
        } catch (const std::exception& e) {
            return std::unexpected(AetherEngine::core::EngineError{2, std::string{"Failed to stop simulation: "} + e.what()});
        } catch (...) {
            return std::unexpected(AetherEngine::core::EngineError{2, "Failed to stop simulation: unknown exception"});
        }
    }

    auto SimulationController::isRunning() const noexcept -> bool {
        return m_state == SimulationState::Running;
    }

    auto SimulationController::cloneScene(AetherEngine::scene::Scene &src) ->
        std::expected<AetherEngine::scene::Scene::ScenePtr, AetherEngine::core::EngineError> {
        using Scene = AetherEngine::scene::Scene;
        using Identity = AetherEngine::components::IdentityComponent;

        try {
            auto copy = Scene::createScene(src.getName());
            auto entities = src.view<Identity>();
            for (auto handle : entities) {
                const auto& identity = entities.get<Identity>(handle);
                const auto source = src.findEntityById(identity.id);
                if (!source || source->handle != handle) {
                    return std::unexpected(AetherEngine::core::EngineError{2, "Failed to clone scene: inconsistent entity identity"});
                }
                auto snapshot = AetherEditor::commands::captureEntity(src, identity.id);
                if (!snapshot || !AetherEditor::commands::restoreEntity(*copy, *snapshot)) {
                    return std::unexpected(AetherEngine::core::EngineError{2, "Failed to clone scene: could not restore entity"});
                }
            }
            return copy;
        } catch (const std::exception &e) {
            return std::unexpected(AetherEngine::core::EngineError{2, std::string{"Failed to clone scene: "} + e.what()});
        }
    }
} // simulation
// AetherEditor
