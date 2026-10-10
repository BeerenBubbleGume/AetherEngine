//
// Created by drhaz on 08.10.2026.
//

#ifndef SMB_VIEWPORTSTATE_HPP
#define SMB_VIEWPORTSTATE_HPP
#include "graphics/RenderTarget.hpp"
#include "math/Types.hpp"


namespace AetherEditor::viewport {
    struct ViewportState {
        AetherEngine::math::Vec2 imageMin{};
        AetherEngine::math::Vec2 imageMax{};

        bool visible = false;
        bool hovered = false;
        bool focused = false;
    };

    struct SceneViewportFrame {
        const AetherEngine::graphics::RenderTarget* target = nullptr;
        ViewportState state{};
    };
} // viewport
// AetherEditor

#endif //SMB_VIEWPORTSTATE_HPP
