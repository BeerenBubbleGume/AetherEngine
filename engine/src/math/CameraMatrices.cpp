//
// Created by drhaz on 26.06.2026.
//

#include "math/CameraMatrices.hpp"

#include <algorithm>

#include <bgfx/bgfx.h>
#include <bx/math.h>

namespace engine::math {
    Mat4 CameraMatrices::makeView(const components::TransformComponent & transform) {
        const auto& cameraTransform = transform.transform;
        const auto eye = cameraTransform.position;

        const Vec3 forward = cameraTransform.rotation.rotate({0.0f, 0.0f, -1.0f});
        const Vec3 up = cameraTransform.rotation.rotate({0.0f, 1.0f, 0.0f});

        return Mat4::lookAtLeftHanded(eye, eye + forward, up);
    }

    Mat4 CameraMatrices::makeProjection(const engine::components::CameraComponent &camera,
        int width, int height) {
        const float safeWidth = static_cast<float>(std::max(width, 1));
        const float safeHeight = static_cast<float>(std::max(height, 1));
        const float aspectRatio = safeWidth / safeHeight;

        Mat4 projection = Mat4::identity();

        const bgfx::Caps* caps = bgfx::getCaps();

        if (camera.projection == components::ProjectionType::Orthographic) {
            const float halfHeight = std::max(camera.orthographicHeight * 0.5f, 0.001f);
            const float halfWidth = halfHeight * aspectRatio;
            bx::mtxOrtho(
                projection.data(),
                -halfWidth,
                halfWidth,
                -halfHeight,
                halfHeight,
                camera.nearPlane,
                camera.farPlane,
                0.0f,
                caps->homogeneousDepth,
                bx::Handedness::Left
            );
        } else {
            bx::mtxProj(
                projection.data(),
                camera.fovYDegrees,
                aspectRatio,
                camera.nearPlane,
                camera.farPlane,
                caps->homogeneousDepth
            );
        }

        return projection;
    }
}
