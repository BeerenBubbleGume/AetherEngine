//
// Created by drhaz on 08.10.2026.
//

#include "panels/HierarchyPanel.hpp"


namespace AetherEditor::panels {
    auto HierarchyPanel::createHierarchyPanel() -> HierarchyPanelPtr {
        return HierarchyPanelPtr(new HierarchyPanel(), HierarchyPanelDeleter{});
    }

    auto HierarchyPanel::draw(bool &open, AetherEngine::scene::Scene& scene, core::EditorSession &session) -> void {
        if (ImGui::Begin("Hierarchy", &open)) {
            using Identity = AetherEngine::components::IdentityComponent;

            auto entities = scene.view<Identity>();

            if (entities.empty()) {
                ImGui::TextUnformatted("Scene is empty");
            }

            for (auto entity : entities) {
                const auto& identity = entities.get<Identity>(entity);
                const auto rowId = AetherEngine::scene::toString(identity.id);

                ImGui::PushID(rowId.c_str());

                const bool selected =
                    session.selectedEntity &&
                    *session.selectedEntity == identity.id;

                const char* label =
                    identity.name.empty() ? "Unnamed entity" : identity.name.c_str();

                if (ImGui::Selectable(label, selected)) {
                    session.selectedEntity = identity.id;
                }

                ImGui::PopID();
            }
        }
        ImGui::End();
    }
} // panels
// AetherEditor