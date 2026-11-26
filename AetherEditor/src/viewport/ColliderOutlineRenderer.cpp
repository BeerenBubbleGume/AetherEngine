//
// Created by Даниил Хазиев on 24.11.2026.
//

#include "viewport/ColliderOutlineRenderer.hpp"

#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <numbers>
#include <vector>

#include "core/ETypes.hpp"
#include "utils/BgfxShaderHelper.hpp"

namespace AetherEditor::viewport {
    namespace {
        struct LineVertex {
            float x, y, z;
        };
        static_assert(sizeof(LineVertex) == 3 * sizeof(float));

        using Lines = std::vector<LineVertex>;
        constexpr int CircleSegments = 48;

        void appendLine(Lines& lines, LineVertex a, LineVertex b) {
            lines.push_back(a);
            lines.push_back(b);
        }

        void appendBoxLines(Lines& lines, const AetherEngine::math::Vec3& halfSize) {
            const auto& [x, y, z] = halfSize;
            const std::array<LineVertex, 8> corners{{
                {-x, -y, -z}, {+x, -y, -z}, {+x, +y, -z}, {-x, +y, -z},
                {-x, -y, +z}, {+x, -y, +z}, {+x, +y, +z}, {-x, +y, +z}
            }};
            constexpr std::array<std::array<unsigned, 2>, 12> edges{{
                {0, 1}, {1, 2}, {2, 3}, {3, 0}, // Face at -Z.
                {4, 5}, {5, 6}, {6, 7}, {7, 4}, // Face at +Z.
                {0, 4}, {1, 5}, {2, 6}, {3, 7}  // Connect the faces.
            }};
            lines.reserve(2 * edges.size());
            for (const auto& [a, b] : edges) {
                appendLine(lines, corners[a], corners[b]);
            }
        }

        // Each step spans the same angle for a full circle and a hemisphere arc.
        template<typename PointAt>
        void appendArc(Lines& lines, float radius, int steps, PointAt pointAt) {
            auto previous = pointAt(radius, 0.0f);
            for (int i = 1; i <= steps; ++i) {
                const float angle = 2.0f * std::numbers::pi_v<float> * i / CircleSegments;
                const auto next = pointAt(radius * std::cos(angle), radius * std::sin(angle));
                appendLine(lines, previous, next);
                previous = next;
            }
        }

        void appendSphereLines(Lines& lines, float radius) {
            lines.reserve(3 * CircleSegments * 2);
            appendArc(lines, radius, CircleSegments, [](float a, float b) { return LineVertex{a, b, 0}; });
            appendArc(lines, radius, CircleSegments, [](float a, float b) { return LineVertex{a, 0, b}; });
            appendArc(lines, radius, CircleSegments, [](float a, float b) { return LineVertex{0, a, b}; });
        }

        void appendCapsuleLines(Lines& lines, float radius, float cylinderHeight) {
            const float halfHeight = cylinderHeight * 0.5f;
            lines.reserve((4 * CircleSegments + 4) * 2);
            for (float side : {-1.0f, 1.0f}) {
                const float x = side * halfHeight;
                appendArc(lines, radius, CircleSegments, [x](float a, float b) {
                    return LineVertex{x, a, b};
                });
                // Two perpendicular half circles bulge outward along local X.
                appendArc(lines, radius, CircleSegments / 2, [x, side](float a, float b) {
                    return LineVertex{x + side * b, a, 0};
                });
                appendArc(lines, radius, CircleSegments / 2, [x, side](float a, float b) {
                    return LineVertex{x + side * b, 0, a};
                });
                appendLine(lines, {-halfHeight, side * radius, 0}, {halfHeight, side * radius, 0});
                appendLine(lines, {-halfHeight, 0, side * radius}, {halfHeight, 0, side * radius});
            }
        }
    }

    auto ColliderOutlineRenderer::create() -> ColliderOutlineRendererPtr {
        return ColliderOutlineRendererPtr(new ColliderOutlineRenderer(), ColliderOutlineRendererDeleter());
    }

    auto ColliderOutlineRenderer::init() -> std::expected<void, AetherEditor::core::EditorError> {
        if (m_initialized) {
            return {};
        }

        m_vertexLayout
            .begin()
            .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
            .end();

        const auto renderer = bgfx::getRendererType();
        const auto [vertexBinary, fragmentBinary] = utils::colliderOutlineShadersFor(renderer);
        if (!vertexBinary.isValid() || !fragmentBinary.isValid()) {
            return std::unexpected(core::EditorError{3, "The active bgfx renderer has no embedded collider outline shaders"});
        }

        auto vertexShader = bgfx::createShader(bgfx::makeRef(vertexBinary.data, vertexBinary.size));
        auto fragmentShader = bgfx::createShader(bgfx::makeRef(fragmentBinary.data, fragmentBinary.size));
        if (!bgfx::isValid(vertexShader) || !bgfx::isValid(fragmentShader)) {
            utils::destroyIfValid(vertexShader);
            utils::destroyIfValid(fragmentShader);
            return std::unexpected(core::EditorError{4, "Failed to create collider outline shaders"});
        }

        m_program = bgfx::createProgram(vertexShader, fragmentShader, false);
        utils::destroyIfValid(vertexShader);
        utils::destroyIfValid(fragmentShader);
        if (!bgfx::isValid(m_program)) {
            return std::unexpected(core::EditorError{5, "Failed to create collider outline shader program"});
        }

        m_initialized = true;
        return {};
    }

    auto ColliderOutlineRenderer::draw(const AetherEngine::graphics::SceneView &view,
                                       const AetherEngine::components::ColliderComponent &collider, const AetherEngine::math::Transform &transform) -> void {
        using AetherEngine::components::ColliderType;
        using AetherEngine::math::Mat4;
        using AetherEngine::math::Vec3;

        if (!m_initialized || !view.target) {
            return;
        }

        Lines vertices;
        switch (collider.type) {
            case ColliderType::Box:
                appendBoxLines(vertices, collider.size);
                break;
            case ColliderType::Sphere:
                appendSphereLines(vertices, collider.radius);
                break;
            case ColliderType::Capsule:
                appendCapsuleLines(vertices, collider.radius, collider.height);
                break;
        }
        if (vertices.empty()) {
            return;
        }
        const auto vertexCount = static_cast<std::uint32_t>(vertices.size());
        if (bgfx::getAvailTransientVertexBuffer(vertexCount, m_vertexLayout) < vertexCount) {
            return;
        }

        bgfx::TransientVertexBuffer buffer;
        bgfx::allocTransientVertexBuffer(&buffer, vertexCount, m_vertexLayout);
        std::memcpy(buffer.data, vertices.data(), vertices.size() * sizeof(LineVertex));

        constexpr bgfx::ViewId OutlineView = 11;
        const auto& rect = view.viewport;
        bgfx::setViewFrameBuffer(OutlineView, view.target->framebuffer());
        bgfx::setViewRect(OutlineView, rect.x, rect.y, rect.width, rect.height);
        bgfx::setViewTransform(OutlineView, view.viewMatrix.data(), view.projectionMatrix.data());
        bgfx::setViewClear(OutlineView, BGFX_CLEAR_NONE);

        auto pose = transform;
        pose.scale = Vec3::one();
        const auto model = Mat4::fromTransform(pose);

        bgfx::setTransform(model.data());
        bgfx::setVertexBuffer(0, &buffer);
        bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_PT_LINES);
        bgfx::submit(OutlineView, m_program);
    }

    auto ColliderOutlineRenderer::shutdown() -> void {
        utils::destroyIfValid(m_program);
        m_program = BGFX_INVALID_HANDLE;
        m_initialized = false;
    }

    ColliderOutlineRenderer::~ColliderOutlineRenderer() {
        shutdown();
    }

    auto ColliderOutlineRenderer::destroyTexture(ImTextureData &texture) -> void {
        auto handle = utils::textureHandle(texture.GetTexID());
        utils::destroyIfValid(handle);
        texture.SetTexID(ImTextureID_Invalid);
        texture.SetStatus(ImTextureStatus_Destroyed);
    }
}
