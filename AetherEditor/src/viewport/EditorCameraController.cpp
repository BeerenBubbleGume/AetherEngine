//
// Created by drhaz on 08.10.2026.
//

#include "viewport/EditorCameraController.hpp"

#include <cmath>

#include "components/CameraComponent.hpp"

namespace AetherEditor::viewport {
    auto EditorCameraController::createEditorCameraController() -> EditorCameraControllerPtr {
        return EditorCameraControllerPtr(new EditorCameraController(), EditorCameraControllerDeleter{});
    }

    auto EditorCameraController::dolly(AetherEngine::components::TransformComponent &transform, AetherEngine::components::CameraComponent &camera, float wheelDelta) const -> void {
        using Projection = AetherEngine::components::ProjectionType;

        if (wheelDelta == 0.0f) {
            return;
        }

        if (camera.projection == Projection::Orthographic) {
            const float zoomFactor = std::exp(-wheelDelta * 0.1f);

            camera.orthographicHeight = std::clamp(
                camera.orthographicHeight * zoomFactor,
                0.1f,
                50.0f
            );

            return;
        }

        const auto forward =
            transform.transform.rotation.rotate({0.0f, 0.0f, -1.0f});

        transform.transform.position +=
            forward * (wheelDelta * m_dollySpeed);
    }

    auto EditorCameraController::look(AetherEngine::components::TransformComponent &camera,
        AetherEngine::math::Vec2 mouseDelta) -> void {
        using Quat = AetherEngine::math::Quat;

        m_yawDegrees += mouseDelta.x * m_lookSensitivity;
        m_pitchDegrees -= mouseDelta.y * m_lookSensitivity;

        m_pitchDegrees = std::clamp(
            m_pitchDegrees,
            -89.0f,
            89.0f);

        const auto yawRotation = Quat::fromAxisAngleDegrees(
            {0.0f, 1.0f, 0.0f},
            m_yawDegrees);
        const auto pitchRotation = Quat::fromAxisAngleDegrees(
            {1.0f, 0.0f, 0.0f},
            m_pitchDegrees);

        camera.transform.rotation = (yawRotation *  pitchRotation).normalized();
    }

    auto EditorCameraController::move(AetherEngine::components::TransformComponent &transform,
                                      const AetherEngine::components::CameraComponent &camera, AetherEngine::math::Vec2 moveAxis, float dt) const -> void {
        using Projection = AetherEngine::components::ProjectionType;

        const auto& rotation = transform.transform.rotation;

        const auto forward = rotation.rotate({0.0f, 0.0f, -1.0f});
        const auto up = rotation.rotate({0.0f, 1.0f, 0.0f});
        const auto right = up.cross(forward).normalized();

        const auto axisYDirection =
            camera.projection == Projection::Orthographic
                ? up
                : forward;

        auto direction =
            right * moveAxis.x +
            axisYDirection * moveAxis.y;

        if (direction.length() > 1.0f) {
            direction = direction.normalized();
        }

        transform.transform.position +=
            direction * (m_movementSpeed * dt);
    }
} // viewport
// AetherEditor