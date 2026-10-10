//
// Created by drhaz on 08.10.2026.
//

#ifndef SMB_EDITORCAMERACONTROLLER_HPP
#define SMB_EDITORCAMERACONTROLLER_HPP
#include <algorithm>
#include <memory>

#include "components/CameraComponent.hpp"
#include "components/TransformComponent.hpp"


namespace AetherEditor::viewport {
    class EditorCameraController {
    public:
        struct EditorCameraControllerDeleter {
            void operator()(EditorCameraController* controller) const {
                delete controller;
            }
        };
        using EditorCameraControllerPtr = std::unique_ptr<EditorCameraController, EditorCameraControllerDeleter>;
        static auto createEditorCameraController() -> EditorCameraControllerPtr;

        EditorCameraController(const EditorCameraController&) = delete;
        EditorCameraController& operator=(const EditorCameraController&) = delete;
        EditorCameraController(EditorCameraController&&) = delete;
        EditorCameraController& operator=(EditorCameraController&&) = delete;
        auto dolly(AetherEngine::components::TransformComponent& transform, AetherEngine::components::CameraComponent &camera, float wheelDelta) const -> void;
        auto look(AetherEngine::components::TransformComponent& camera, AetherEngine::math::Vec2 mouseDelta) -> void;
        auto move(AetherEngine::components::TransformComponent& transform, const AetherEngine::components::CameraComponent &camera, AetherEngine::math::Vec2 moveAxis, float dt) const -> void;
    private:
        EditorCameraController() = default;
        ~EditorCameraController() = default;

        const float m_dollySpeed = 0.5f;
        float m_movementSpeed = 5.0f;
        float m_yawDegrees = 0.0f;
        float m_pitchDegrees = 0.0f;
        float m_lookSensitivity = 0.15f;
    };
} // viewport
// AetherEditor

#endif //SMB_EDITORCAMERACONTROLLER_HPP
