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
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace engine::scene {
    class SCCamera {
    public:
        SCCamera() = default;
        ~SCCamera() = default;

        SCCamera(const SCCamera& other);
        SCCamera& operator=(const SCCamera& other);
        SCCamera(SCCamera&& other) noexcept = default;
        SCCamera& operator=(SCCamera&& other) noexcept = default;

        [[nodiscard]] auto getViewMatrix() -> const glm::mat4 &;
        [[nodiscard]] auto getProjectionMatrix(std::tuple<int, int> viewShape) -> const glm::mat4 &;

        auto setTransform(const graphics::GTransform& transform) -> void;
    private:
        graphics::GTransform m_transform;
        //std::array<float, 16> m_viewMatrix{};
        //std::array<float, 16> m_projectionMatrix{};
        glm::mat4 m_viewMatrix {1};
        glm::mat4 m_projectionMatrix {1};
    };
} // scene
// engine

#endif //SMB_SCAMERA_HPP
