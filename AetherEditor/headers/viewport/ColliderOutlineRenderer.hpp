//
// Created by Даниил Хазиев on 24.11.2026.
//

#ifndef AETHER_EDITOR_COLLIDERRENDERER_HPP
#define AETHER_EDITOR_COLLIDERRENDERER_HPP
#include <imgui.h>
#include <memory>

#include "components/PhysicsComponents.hpp"
#include "graphics/SceneView.hpp"

namespace AetherEditor::core {
    struct EditorError;
}

namespace AetherEditor::viewport {
    class ColliderOutlineRenderer {
    public:
        struct ColliderOutlineRendererDeleter {
            void operator()(ColliderOutlineRenderer* ptr) {
                delete ptr;
            }
        };
        using ColliderOutlineRendererPtr = std::unique_ptr<ColliderOutlineRenderer, ColliderOutlineRendererDeleter>;
        [[nodiscard]] static auto create() -> ColliderOutlineRendererPtr;
        [[nodiscard]] auto init() -> std::expected<void, AetherEditor::core::EditorError>;
        auto draw(const AetherEngine::graphics::SceneView &view, const AetherEngine::components::ColliderComponent &collider, const
                  AetherEngine::math::Transform &transform) -> void;
        auto shutdown() -> void;
    private:
        ColliderOutlineRenderer() = default;
        ~ColliderOutlineRenderer();
        auto destroyTexture(ImTextureData& texture) -> void;
        bool m_initialized = false;
        bgfx::ProgramHandle m_program;
        bgfx::VertexLayout m_vertexLayout;
    };
}

#endif //AETHER_EDITOR_COLLIDERRENDERER_HPP
