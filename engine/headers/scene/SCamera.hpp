//
// Created by drhaz on 21.06.2026.
//

#ifndef SMB_SCAMERA_HPP
#define SMB_SCAMERA_HPP
#include <array>
#include <span>
#include <bgfx/bgfx.h>
#include <bx/math.h>

#include "graphics/GMesh.hpp"


namespace engine::scene {
    class SCamera {
    public:
        SCamera() = default;
        ~SCamera() = default;

        SCamera(const SCamera& other);
        SCamera& operator=(const SCamera& other);
        SCamera(SCamera&& other) noexcept = default;
        SCamera& operator=(SCamera&& other) noexcept = default;

        [[nodiscard]] auto getViewMatrix() -> const std::array<float, 16> &;
        [[nodiscard]] auto getProjectionMatrix(std::tuple<int, int> viewShape) -> const std::array<float, 16> &;

        auto setTransform(const graphics::GTransform& transform) -> void;
    private:
        graphics::GTransform m_transform;
        std::array<float, 16> m_viewMatrix{};
        std::array<float, 16> m_projectionMatrix{};
    };
} // scene
// engine

#endif //SMB_SCAMERA_HPP
