//
// Created by drhaz on 21.06.2026.
//

#ifndef SMB_SCAMERA_HPP
#define SMB_SCAMERA_HPP
#include <tuple>

#include "graphics/GMesh.hpp"
#include "math/UTypes.hpp"

namespace engine::scene {
    class SCCamera final {
    public:
        SCCamera() = default;
        ~SCCamera() = default;

        SCCamera(const SCCamera& other);
        SCCamera& operator=(const SCCamera& other);
        SCCamera(SCCamera&& other) noexcept = default;
        SCCamera& operator=(SCCamera&& other) noexcept = default;

        [[nodiscard]] auto getViewMatrix() -> const math::TMat4 &;
        [[nodiscard]] auto getProjectionMatrix(std::tuple<int, int> viewShape) -> const math::TMat4 &;

        auto setTransform(const graphics::GTransform& transform) -> void;
    private:
        graphics::GTransform m_transform;
        math::TMat4 m_viewMatrix {1.0f};
        math::TMat4 m_projectionMatrix {1.0f};
    };
} // scene
// engine

#endif //SMB_SCAMERA_HPP
