//
// Created by drhaz on 08.10.2026.
//

#ifndef SMB_SCENEVIEWPORTPANEL_HPP
#define SMB_SCENEVIEWPORTPANEL_HPP
#include <memory>
#include <imgui.h>
#include <graphics/RenderTarget.hpp>
#include <optional>
#include <algorithm>
#include <cmath>
#include <limits>


#include "components/CameraComponent.hpp"
#include "viewport/ViewportState.hpp"

namespace AetherEditor::panels {
    class SceneViewportPanel {
    public:
        struct SceneViewportPanelDeleter {
            void operator()(SceneViewportPanel* panel) const {
                delete panel;
            }
        };
        using SceneViewportPanelPtr = std::unique_ptr<SceneViewportPanel, SceneViewportPanelDeleter>;
        static auto createSceneViewportPanel() -> SceneViewportPanelPtr;
        auto draw(bool &open, AetherEngine::components::CameraComponent &camera) -> viewport::SceneViewportFrame;

        SceneViewportPanel(const SceneViewportPanel&) = delete;
        SceneViewportPanel& operator=(const SceneViewportPanel&) = delete;
        SceneViewportPanel(SceneViewportPanel&&) = delete;
        SceneViewportPanel& operator=(SceneViewportPanel&&) = delete;
    private:
        SceneViewportPanel() = default;
        ~SceneViewportPanel() = default;
        AetherEngine::graphics::RenderTarget m_target;
    };
} // panels
// AetherEditor

#endif //SMB_SCENEVIEWPORTPANEL_HPP
