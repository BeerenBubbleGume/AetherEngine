//
// Created by drhaz on 08.10.2026.
//

#include "panels/InspectorPanel.hpp"


namespace AetherEditor::panels {
    auto InspectorPanel::createInspectorPanel() -> InspectorPanelPtr {
        return InspectorPanelPtr(new InspectorPanel(), InspectorPanelDeleter());
    }

    auto InspectorPanel::draw(bool &open) -> void {
        if (ImGui::Begin("Inspector", &open)) {
            ImGui::TextUnformatted("Selected entity properties");
        }
        ImGui::End();
    }
} // panel
// AetherEditor