//
// Created by drhaz on 21.06.2026.
//

#include "scene/SCCamera.hpp"

#include <algorithm>

#include <bgfx/bgfx.h>
#include <bx/math.h>


namespace engine::scene {
    SCCamera::SCCamera(const SCCamera &other) {
        m_transform = other.m_transform;
        m_viewMatrix = other.m_viewMatrix;
        m_projectionMatrix = other.m_projectionMatrix;

        *this = other;
    }

    SCCamera & SCCamera::operator=(const SCCamera &other) = default;

    auto SCCamera::getViewMatrix() -> const math::TMat4 & {
        const auto eye = m_transform.position;
        const math::TVec3 at{0.0f, 0.0f, 0.0f};
        const math::TVec3 up{0.0f, 1.0f, 0.0f};

        m_viewMatrix = math::TMat4::lookAtLeftHanded(eye, at, up);
        return m_viewMatrix;
    }

    auto SCCamera::getProjectionMatrix(std::tuple<int, int> viewShape) -> const math::TMat4 & {
        const auto [width, height] = viewShape;
        const float safeWidth = static_cast<float>(std::max(width, 1));
        const float safeHeight = static_cast<float>(std::max(height, 1));
        const float aspectRatio = safeWidth / safeHeight;

        const bgfx::Caps* caps = bgfx::getCaps();

        bx::mtxProj(
            m_projectionMatrix.data(),
            60.0f,
            aspectRatio,
            0.1f,
            100.0f,
            caps->homogeneousDepth
        );

        return m_projectionMatrix;
    }

    auto SCCamera::setTransform(const graphics::Transform &transform) -> void {
        m_transform = transform;
    }
} // scene
// engine
