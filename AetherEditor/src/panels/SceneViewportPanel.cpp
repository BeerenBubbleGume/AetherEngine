//
// Created by drhaz on 08.10.2026.
//

#include "panels/SceneViewportPanel.hpp"

#include "ui/ImGuiBgfxRenderer.hpp"

namespace {

    [[nodiscard]]
    auto makeRenderExtent(
        const ImVec2 imageSize,
        const ImVec2 framebufferScale
    ) -> std::optional<AetherEngine::graphics::RenderExtent> {
        const bool validInput =
            std::isfinite(imageSize.x) &&
            std::isfinite(imageSize.y) &&
            std::isfinite(framebufferScale.x) &&
            std::isfinite(framebufferScale.y) &&
            imageSize.x > 0.0f &&
            imageSize.y > 0.0f &&
            framebufferScale.x > 0.0f &&
            framebufferScale.y > 0.0f;

        if (!validInput) {
            return std::nullopt;
        }

        const float pixelWidthF =
            imageSize.x * framebufferScale.x;

        const float pixelHeightF =
            imageSize.y * framebufferScale.y;

        const auto* caps = bgfx::getCaps();

        if (caps == nullptr) {
            return std::nullopt;
        }

        const auto maxExtent = std::min<std::uint32_t>(
            caps->limits.maxTextureSize,
            std::numeric_limits<std::uint16_t>::max()
        );

        const bool validExtent =
            std::isfinite(pixelWidthF) &&
            std::isfinite(pixelHeightF) &&
            pixelWidthF >= 1.0f &&
            pixelHeightF >= 1.0f &&
            pixelWidthF <= static_cast<float>(maxExtent) &&
            pixelHeightF <= static_cast<float>(maxExtent);

        if (!validExtent) {
            return std::nullopt;
        }

        return AetherEngine::graphics::RenderExtent{
            .width = static_cast<std::uint16_t>(
                std::lround(pixelWidthF)
            ),
            .height = static_cast<std::uint16_t>(
                std::lround(pixelHeightF)
            )
        };
    }

}

namespace AetherEditor::panels {
    auto SceneViewportPanel::createSceneViewportPanel() -> SceneViewportPanelPtr {
        return SceneViewportPanelPtr(new SceneViewportPanel(), SceneViewportPanelDeleter{});
    }

    auto SceneViewportPanel::draw(bool &open, AetherEngine::components::CameraComponent &camera) -> viewport::SceneViewportFrame {
        viewport::SceneViewportFrame result;
        if (ImGui::Begin("Scene Viewport###SceneViewport", &open)) {
            using Projection = AetherEngine::components::ProjectionType;

            const char* projectionNames[] = {
                "Perspective",
                "Orthographic"
            };

            int projectionIndex = static_cast<int>(camera.projection);

            ImGui::SetNextItemWidth(150.0f);

            if (ImGui::Combo(
                "Projection",
                &projectionIndex,
                projectionNames,
                2
            )) {
                camera.projection = static_cast<Projection>(projectionIndex);
            }

            ImGui::SameLine();
            ImGui::SetNextItemWidth(180.0f);

            if (camera.projection == Projection::Perspective) {
                ImGui::SliderFloat(
                    "FOV",
                    &camera.fovYDegrees,
                    30.0f,
                    90.0f,
                    "%.0f deg",
                    ImGuiSliderFlags_AlwaysClamp
                );
            } else {
                ImGui::SliderFloat(
                    "View height",
                    &camera.orthographicHeight,
                    0.1f,
                    50.0f,
                    "%.2f",
                    ImGuiSliderFlags_AlwaysClamp
                );
            }

            ImGui::Separator();
            const ImVec2 imageSize = ImGui::GetContentRegionAvail();
            const ImVec2 framebufferScale = ImGui::GetIO().DisplayFramebufferScale;
            const auto desiredExtent = makeRenderExtent(imageSize, framebufferScale);

            if (!desiredExtent.has_value()) {
                ImGui::TextUnformatted("Viewport size is invalid");
            } else {
                const auto resizeResult = m_target.resize(*desiredExtent);

                if (!resizeResult) {
                    ImGui::TextUnformatted("Failed to create viewport render target");
                } else {
                    const bgfx::TextureHandle colorTexture = m_target.colorTexture();

                    if (!bgfx::isValid(colorTexture)) {
                        ImGui::TextUnformatted("Viewport color texture is invalid");
                    } else {
                        const auto textureId = ui::ImGuiBgfxRenderer::textureId(colorTexture);

                        const bool flipY = bgfx::getCaps()->originBottomLeft;

                        const ImVec2 uv0 = flipY
                            ? ImVec2{0.0f, 1.0f}
                            : ImVec2{0.0f, 0.0f};

                        const ImVec2 uv1 = flipY
                            ? ImVec2{1.0f, 0.0f}
                            : ImVec2{1.0f, 1.0f};

                        ImGui::Image(
                            ImTextureRef(textureId),
                            imageSize,
                            uv0,
                            uv1
                        );

                        const auto imageMin = ImGui::GetItemRectMin();
                        const auto imageMax = ImGui::GetItemRectMax();

                        result.target = &m_target;
                        result.state.imageMin = {imageMin.x, imageMin.y};
                        result.state.imageMax = {imageMax.x, imageMax.y};
                        result.state.visible = ImGui::IsItemVisible();
                        result.state.hovered = ImGui::IsItemHovered();
                        result.state.focused = ImGui::IsWindowFocused();
                    }
                }
            }
        }
        ImGui::End();

        return result;
    }
} // panels
// AetherEditor