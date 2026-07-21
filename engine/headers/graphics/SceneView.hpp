#ifndef SMB_SCENEVIEW_HPP
#define SMB_SCENEVIEW_HPP

#include <cstdint>

#include <bgfx/bgfx.h>

#include "graphics/RenderTarget.hpp"
#include "math/Types.hpp"

namespace engine::graphics {
    struct RenderViewport {
        uint16_t x{0};
        uint16_t y{0};
        uint16_t width{1};
        uint16_t height{1};
    };

    // Non-owning, per-frame description of one scene render pass.
    struct SceneView {
        bgfx::ViewId viewId{0};
        // A null target renders into the main backbuffer.
        const RenderTarget* target{nullptr};

        math::Mat4 viewMatrix{math::Mat4::identity()};
        math::Mat4 projectionMatrix{math::Mat4::identity()};
        RenderViewport viewport{};

        uint16_t clearFlags{BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH};
        uint32_t clearColor{0x303030ff};
        float clearDepth{1.0f};
        uint8_t clearStencil{0};
    };
}

#endif // SMB_SCENEVIEW_HPP
