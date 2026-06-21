//
// Created by drhaz on 21.06.2026.
//

#include "scene/SCamera.hpp"


namespace engine::scene {
    SCamera::SCamera(const SCamera &other) {
        m_transform = other.m_transform;
        m_viewMatrix = other.m_viewMatrix;
        m_projectionMatrix = other.m_projectionMatrix;

        *this = other;
    }

    SCamera & SCamera::operator=(const SCamera &other) = default;

    auto SCamera::getViewMatrix() -> const std::array<float, 16> & {
        bx::mtxLookAt(m_viewMatrix.data(),
            { 0.0f, 0.0f, 5.0f },
            { 0.0f, 0.0f, 0.0f },
            { 0.0f, 1.0f, 0.0f }
        );
        return m_viewMatrix;
    }

    auto SCamera::getProjectionMatrix(std::tuple<int, int> viewShape) -> const std::array<float, 16> & {
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

    auto SCamera::setTransform(const graphics::GTransform &transform) -> void {
        m_transform = transform;
    }
} // scene
// engine