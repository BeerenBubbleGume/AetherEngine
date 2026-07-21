//
// Created by drhaz on 21.06.2026.
//

#ifndef SMB_SCENE_CAMERA_HPP
#define SMB_SCENE_CAMERA_HPP
#include <tuple>

#include "graphics/Mesh.hpp"
#include "math/Types.hpp"

namespace engine::scene {
    class Camera final {
    public:
        Camera() = default;
        ~Camera() = default;

        Camera(const Camera& other);
        Camera& operator=(const Camera& other);
        Camera(Camera&& other) noexcept = default;
        Camera& operator=(Camera&& other) noexcept = default;

        [[nodiscard]] auto getViewMatrix() -> const math::Mat4 &;
        [[nodiscard]] auto getProjectionMatrix(std::tuple<int, int> viewShape) -> const math::Mat4 &;

        auto setTransform(const graphics::Transform& transform) -> void;
    private:
        graphics::Transform m_transform;
        math::Mat4 m_viewMatrix {1.0f};
        math::Mat4 m_projectionMatrix {1.0f};
    };
} // scene
// engine

#endif //SMB_SCENE_CAMERA_HPP
