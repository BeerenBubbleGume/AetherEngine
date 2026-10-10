//
// Created by drhaz on 08.10.2026.
//

#ifndef SMB_EDITORINPUTROUTER_HPP
#define SMB_EDITORINPUTROUTER_HPP
#include <memory>

#include "viewport/ViewportState.hpp"
#include "systems/InputSystem.hpp"

namespace AetherEditor::input {
    struct CameraInput {
        float dolly = 0.0f;
        AetherEngine::math::Vec2 lookDelta{};
        AetherEngine::math::Vec2 moveAxis{};
        bool looking = false;
    };

    class EditorInputRouter {
    public:
        struct EditorInputRouterDeleter {
            void operator()(EditorInputRouter* router) const {
                delete router;
            }
        };
        using EditorInputRouterPtr = std::unique_ptr<EditorInputRouter, EditorInputRouterDeleter>;
        static auto createEditorInputRouter() -> EditorInputRouterPtr;

        EditorInputRouter(const EditorInputRouter&) = delete;
        EditorInputRouter& operator=(const EditorInputRouter&) = delete;
        EditorInputRouter(EditorInputRouter&&) = delete;
        EditorInputRouter& operator=(EditorInputRouter&&) = delete;
        auto route(const viewport::ViewportState &state, const AetherEngine::systems::InputSystem &input, bool applicationFocused) -> CameraInput;
        auto cancel() -> void;
    private:
        EditorInputRouter() = default;
        ~EditorInputRouter() = default;
        bool m_looking = false;
    };
} // input
// AetherEditor

#endif //SMB_EDITORINPUTROUTER_HPP
