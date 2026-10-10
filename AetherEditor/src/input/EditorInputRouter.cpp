//
// Created by drhaz on 08.10.2026.
//

#include "input/EditorInputRouter.hpp"


namespace AetherEditor::input {
    auto EditorInputRouter::createEditorInputRouter() -> EditorInputRouterPtr {
        return EditorInputRouterPtr(new EditorInputRouter(), EditorInputRouterDeleter{});
    }

    auto EditorInputRouter::route(const AetherEditor::viewport::ViewportState &state, const AetherEngine::systems::InputSystem &input, bool applicationFocused) -> AetherEditor::input::CameraInput {
        using MouseButton = AetherEngine::systems::MouseButton;
        using Action = AetherEngine::systems::InputAction;
        using Vec2 = AetherEngine::math::Vec2;

        if (!applicationFocused || !state.visible) {
            cancel();
            return {};
        }
        const bool rightDown = input.isMouseButtonDown(MouseButton::Right);

        if (!rightDown || input.wasMouseButtonReleased(MouseButton::Right)) {
            cancel();
        }

        const bool startedLooking = !m_looking && state.hovered && input.wasMouseButtonPressed(MouseButton::Right) && rightDown;
        if (startedLooking) {
            m_looking = true;
        }
        Vec2 moveAxis{};

        if (m_looking) {
            if (input.isActionDown(Action::CameraMoveRight))
                moveAxis.x += 1.0f;

            if (input.isActionDown(Action::CameraMoveLeft))
                moveAxis.x -= 1.0f;

            if (input.isActionDown(Action::CameraMoveForward))
                moveAxis.y += 1.0f;

            if (input.isActionDown(Action::CameraMoveBackward))
                moveAxis.y -= 1.0f;
        }
        return CameraInput{
            .dolly = state.hovered ? input.mouseWheelY() : 0.0f,
            .lookDelta = (m_looking && !startedLooking)
                ? Vec2{input.mouseDeltaX(), input.mouseDeltaY()}
                : Vec2{},
            .moveAxis = moveAxis,
            .looking = m_looking
        };
    }

    auto EditorInputRouter::cancel() -> void {
        m_looking = false;
    }
} // input
// AetherEditor