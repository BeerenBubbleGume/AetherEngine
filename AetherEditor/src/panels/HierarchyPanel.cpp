//
// Created by drhaz on 08.10.2026.
//

#include "panels/HierarchyPanel.hpp"

#include <imgui_stdlib.h>
#include "core/EntityName.hpp"

namespace AetherEditor::panels {
    auto HierarchyPanel::createHierarchyPanel() -> HierarchyPanelPtr {
        return HierarchyPanelPtr(new HierarchyPanel(), HierarchyPanelDeleter{});
    }

    auto HierarchyPanel::draw(bool &open, AetherEngine::scene::Scene& scene, core::EditorSession &session) -> HierarchyFrame {
        HierarchyFrame frame{};
        if (m_rename) {
            const auto entity = scene.findEntityById(m_rename->entityId);
            if (m_rename->scene != &scene || !entity || session.selectedEntity != m_rename->entityId ||
                scene.getComponent<AetherEngine::components::IdentityComponent>(*entity).name != m_rename->before) {
                cancelRename();
            }
        }
        if (ImGui::Begin("Hierarchy", &open) && open) {
            frame.createEntityRequested = ImGui::Button("Create Entity");
            ImGui::Separator();

            using Identity = AetherEngine::components::IdentityComponent;

            auto entities = scene.view<Identity>();

            if (entities.empty()) {
                ImGui::TextUnformatted("Scene is empty");
            }

            for (auto entity : entities) {
                const auto& identity = entities.get<Identity>(entity);
                const auto rowId = AetherEngine::scene::toString(identity.id);

                ImGui::PushID(rowId.c_str());

                drawEntityRow(scene, identity, session, frame);

                ImGui::PopID();
            }
        } else {
            cancelRename();
        }
        ImGui::End();
        return frame;
    }

    auto HierarchyPanel::drawEntityRow(AetherEngine::scene::Scene& scene,
                                      const AetherEngine::components::IdentityComponent& identity,
                                      core::EditorSession& session, HierarchyFrame& frame) -> void {
        if (m_rename && m_rename->entityId == identity.id) {
            drawRenameInput(frame);
            return;
        }

        const bool selected = session.selectedEntity == identity.id;
        auto textPosition = ImGui::GetCursorScreenPos();
        textPosition.y += ImGui::GetStyle().FramePadding.y;
        if (ImGui::Selectable("##Entity", selected, ImGuiSelectableFlags_AllowDoubleClick,
                              ImVec2{0.0f, ImGui::GetFrameHeight()})) {
            session.selectedEntity = identity.id;
            if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                finishRename(frame);
                m_rename = PendingRename{&scene, identity.id, identity.name, identity.name};
            }
        }
        // Keep the widget ID independent of the name and display literal '#' characters.
        const char* label = identity.name.empty() ? "Unnamed entity" : identity.name.c_str();
        ImGui::GetWindowDrawList()->AddText(textPosition, ImGui::GetColorU32(ImGuiCol_Text), label);
    }

    auto HierarchyPanel::drawRenameInput(HierarchyFrame& frame) -> void {
        if (m_rename->focusRequested) {
            ImGui::SetKeyboardFocusHere();
            m_rename->focusRequested = false;
        }
        ImGui::SetNextItemWidth(-1.0f);
        const bool submitted = ImGui::InputText("##Rename", &m_rename->name,
            ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll);
        const bool deactivated = ImGui::IsItemDeactivated();
        if (ImGui::IsKeyPressed(ImGuiKey_Escape, false)) {
            cancelRename();
        } else if (submitted || deactivated) {
            if (submitted && !core::isValidEntityName(m_rename->name)) {
                m_rename->focusRequested = true;
            } else {
                finishRename(frame);
            }
        }
        if (m_rename && !core::isValidEntityName(m_rename->name)) {
            ImGui::TextDisabled(m_rename->name.size() > 1024 ? "Name is too long." : "Name cannot be blank.");
        }
    }

    auto HierarchyPanel::finishRename(HierarchyFrame& frame) -> void {
        if (m_rename && core::isValidEntityName(m_rename->name) && m_rename->name != m_rename->before) {
            frame.renameEntityRequested = RenameEntityRequest{m_rename->entityId, std::move(m_rename->name)};
        }
        cancelRename();
    }

    auto HierarchyPanel::cancelRename() -> void {
        m_rename.reset();
    }

    auto HierarchyPanel::isRenaming() const -> bool {
        return m_rename.has_value();
    }
} // panels
// AetherEditor
