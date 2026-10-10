//
// Created by drhaz on 09.07.2026.
//

#ifndef SMB_AETHEREDITOR_HPP
#define SMB_AETHEREDITOR_HPP
#include <memory>

#include "components/CameraComponent.hpp"
#include "components/TransformComponent.hpp"
#include "core/IApplication.hpp"
#include "ETypes.hpp"
#include <ui/EditorGui.hpp>
#include <ui/EditorWorkspace.hpp>
#include <input/EditorInputRouter.hpp>
#include <viewport/EditorCameraController.hpp>

namespace AetherEditor::core {
    class Editor final : public AetherEngine::core::IApplication {
    public:
        using AetherEditorPtr = std::unique_ptr<Editor>;

        ~Editor() override = default;

        auto init(AetherEngine::core::EngineContext &ctx) -> std::expected<void, AetherEngine::core::EngineError> override;
        auto onEvent(const SDL_Event& event) -> void override;
        auto update(float dt, AetherEngine::core::EngineContext &ctx) -> void override;
        auto render(AetherEngine::core::EngineContext& ctx) -> void override;
        auto shutdown(AetherEngine::core::EngineContext& ctx) -> void override;

        auto saveAs(AetherEngine::core::EngineContext& ctx, std::string_view filename) -> std::expected<void, AetherEngine::core::EngineError>;
        auto save(AetherEngine::core::EngineContext& ctx) -> std::expected<void, AetherEngine::core::EngineError>;
        auto load(AetherEngine::core::EngineContext& ctx, std::string_view filename) -> std::expected<void, AetherEngine::core::EngineError>;
        auto newScene(AetherEngine::core::EngineContext& ctx, std::string_view name) -> std::expected<void, AetherEngine::core::EngineError>;

        [[nodiscard]] static auto createEditor() -> AetherEditorPtr;
        [[nodiscard]] auto runConfig() const -> AetherEngine::core::EngineRunConfig override;
    private:
        Editor() = default;
        using EditorGuiPtr = ui::EditorGui::EditorGuiPtr;
        using EditorWorkspacePtr = ui::EditorWorkspace::EditorWorkspacePtr;
        using InputRouterPtr  = input::EditorInputRouter::EditorInputRouterPtr;
        using CameraControllerPtr = viewport::EditorCameraController::EditorCameraControllerPtr;
        AetherEngine::components::TransformComponent m_editorCameraTransform{};
        AetherEngine::components::CameraComponent m_editorCamera{};

        EditorGuiPtr m_gui;
        EditorWorkspacePtr m_workspace;
        InputRouterPtr m_inputRouter;
        CameraControllerPtr m_cameraController;
        SceneEditorDocument m_document{};
        float m_frameDelta = 0.0f;
        bool m_relativeMouseMode = false;

        auto setRelativeMouseMode(SDL_Window* window, bool enabled) -> bool;
    };
}


#endif //SMB_AETHEREDITOR_HPP
