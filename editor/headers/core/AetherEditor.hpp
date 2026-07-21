//
// Created by drhaz on 09.07.2026.
//

#ifndef SMB_AETHEREDITOR_HPP
#define SMB_AETHEREDITOR_HPP
#include <memory>

#include "components/CameraComponent.hpp"
#include "components/TransformComponent.hpp"
#include "core/IApplication.hpp"
#include "core/ETypes.hpp"
#include "ui/EditorGui.hpp"

namespace AetherEditor::core {
    class Editor final : public engine::core::IApplication {
    public:
        using AetherEditorPtr = std::unique_ptr<Editor>;

        ~Editor() override = default;

        auto init(engine::core::EngineContext &ctx) -> std::expected<void, engine::core::EngineError> override;
        auto onEvent(const SDL_Event& event) -> void override;
        auto update(float dt, engine::core::EngineContext &ctx) -> void override;
        auto render(engine::core::EngineContext& ctx) -> void override;
        auto shutdown(engine::core::EngineContext& ctx) -> void override;

        [[nodiscard]] static auto createEditor() -> AetherEditorPtr;
        [[nodiscard]] auto runConfig() const -> engine::core::EngineRunConfig override;
    private:
        Editor() = default;
        using EditorGuiPtr = std::unique_ptr<ui::EditorGui, ui::EditorGui::EditorGuiDeleter>;

        engine::components::TransformComponent m_editorCameraTransform{};
        engine::components::CameraComponent m_editorCamera{};

        EditorGuiPtr m_gui;
    };
}


#endif //SMB_AETHEREDITOR_HPP
