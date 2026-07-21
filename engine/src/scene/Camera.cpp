//
// Created by drhaz on 21.06.2026.
//

#include "scene/Camera.hpp"

#include <algorithm>

#include <bgfx/bgfx.h>
#include <bx/math.h>


namespace engine::scene {
    Camera::Camera(const Camera &other) {
        m_transform = other.m_transform;
        m_viewMatrix = other.m_viewMatrix;
        m_projectionMatrix = other.m_projectionMatrix;

        *this = other;
    }

    Camera & Camera::operator=(const Camera &other) = default;

    auto Camera::getViewMatrix() -> const math::Mat4 & {
        const auto eye = m_transform.position;
        const math::Vec3 at{0.0f, 0.0f, 0.0f};
        const math::Vec3 up{0.0f, 1.0f, 0.0f};

        m_viewMatrix = math::Mat4::lookAtLeftHanded(eye, at, up);
        return m_viewMatrix;
    }

    auto Camera::getProjectionMatrix(std::tuple<int, int> viewShape) -> const math::Mat4 & {
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

    auto Camera::setTransform(const graphics::Transform &transform) -> void {
        m_transform = transform;
    }
} // scene
// engine
