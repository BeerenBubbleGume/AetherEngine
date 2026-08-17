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
#include "ui/EditorGui.hpp"

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

        [[nodiscard]] static auto createEditor() -> AetherEditorPtr;
        [[nodiscard]] auto runConfig() const -> AetherEngine::core::EngineRunConfig override;
    private:
        Editor() = default;
        using EditorGuiPtr = std::unique_ptr<ui::EditorGui, ui::EditorGui::EditorGuiDeleter>;

        AetherEngine::components::TransformComponent m_editorCameraTransform{};
        AetherEngine::components::CameraComponent m_editorCamera{};

        EditorGuiPtr m_gui;
    };
}


#endif //SMB_AETHEREDITOR_HPP
