//
// Created by drhaz on 08.10.2026.
//

#include "panels/HierarchyPanel.hpp"


namespace AetherEditor::panels {
    auto HierarchyPanel::createHierarchyPanel() -> HierarchyPanelPtr {
        return HierarchyPanelPtr(new HierarchyPanel(), HierarchyPanelDeleter{});
    }

    auto HierarchyPanel::draw(bool &open) -> void {
        if (ImGui::Begin("Hierarchy", &open)) {
            ImGui::TextUnformatted("Scene entities");
        }
        ImGui::End();
    }
} // panels
// AetherEditor