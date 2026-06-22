//
// Created by drhaz on 21.06.2026.
//

#include "scene/SCCamera.hpp"


namespace engine::scene {
    SCCamera::SCCamera(const SCCamera &other) {
        m_transform = other.m_transform;
        m_viewMatrix = other.m_viewMatrix;
        m_projectionMatrix = other.m_projectionMatrix;

        *this = other;
    }

    SCCamera & SCCamera::operator=(const SCCamera &other) = default;

    auto SCCamera::getViewMatrix() -> const std::array<float, 16> & {
        const auto eye = m_transform.position;
        const bx::Vec3 at = {0.0f, 0.0f, 0.0f};
        const bx::Vec3 up = {0.0f, 1.0f, 0.0f};

        bx::mtxLookAt(m_viewMatrix.data(), eye, at, up);

        return m_viewMatrix;
    }

    auto SCCamera::getProjectionMatrix(std::tuple<int, int> viewShape) -> const std::array<float, 16> & {
        const auto [width, height] = viewShape;
        const auto caps = bgfx::getCaps();
        bx::mtxProj(m_projectionMatrix.data(),
            60.0f,
            float(width) / float(height),
            0.1f, 100.0f,
            caps->homogeneousDepth
        );
        return m_projectionMatrix;
    }

    auto SCCamera::setTransform(const graphics::GTransform &transform) -> void {
        m_transform = transform;
    }
} // scene
// engine