//
// Created by drhaz on 08.10.2026.
//

#include "panels/InspectorPanel.hpp"

#include "commands/ComponentCommands.hpp"
#include "components/TransformComponent.hpp"
#include "core/EditorSession.hpp"
#include "scene/Scene.hpp"


namespace AetherEditor::panels {
    auto InspectorPanel::createInspectorPanel() -> InspectorPanelPtr {
        return InspectorPanelPtr(new InspectorPanel(), InspectorPanelDeleter());
    }

    auto InspectorPanel::draw(bool &open, AetherEngine::scene::Scene& scene, const core::EditorSession& session, commands::CommandHistory& history) ->
        bool {
        using Identity = AetherEngine::components::IdentityComponent;
        using Transform = AetherEngine::components::TransformComponent;
        using Quat = AetherEngine::math::Quat;

        bool changed = false;
        bool editActiveThisFrame = false;
        if (ImGui::Begin("Inspector", &open)) {
            if (!session.selectedEntity) {
                ImGui::TextUnformatted("No entity selected");
            } else if (const auto entity = scene.findEntityById(*session.selectedEntity)) {
                const auto& identity = scene.getComponent<Identity>(*entity);

                ImGui::TextUnformatted(identity.name.c_str());
                ImGui::Separator();

                if (auto* component = scene.tryGetComponent<Transform>(*entity)) {
                    auto& t = component->transform;
                    auto beginEdit = [&](const AetherEngine::math::Transform& before) {
                        if (ImGui::IsItemActivated()) {
                            finishTransformEdit(scene, history);

                            m_transformEdit = PendingTransformEdit{
                                identity.id,
                                ImGui::GetItemID(),
                                before
                            };
                        }

                        if (m_transformEdit &&
                            m_transformEdit->widgetId == ImGui::GetItemID() &&
                            ImGui::IsItemActive()) {
                            editActiveThisFrame = true;
                            }
                    };

                    auto endEdit = [&] {
                        if (m_transformEdit &&
                            m_transformEdit->widgetId == ImGui::GetItemID() &&
                            ImGui::IsItemDeactivated() &&
                            !ImGui::IsItemActive()) {
                            finishTransformEdit(scene, history);
                            }
                    };

                    const auto widgetId = AetherEngine::scene::toString(identity.id);
                    ImGui::PushID(widgetId.c_str());

                    ImGui::TextUnformatted("Transform");

                    float position[3]{
                        t.position.x, t.position.y, t.position.z
                    };

                    const auto beforePosition = t;
                    const bool positionChanged = ImGui::DragFloat3("Position", position, 0.05f);

                    beginEdit(beforePosition);

                    if (positionChanged) {
                        t.position = {position[0], position[1], position[2]};
                        changed = true;
                    }

                    endEdit();

                    if (!m_rotationEntity || *m_rotationEntity != identity.id || m_rotationQuaternion != t.rotation) {
                        m_rotationEntity = identity.id;
                        m_rotationDegrees = t.rotation.toEulerXYZDegrees();
                        m_rotationQuaternion = t.rotation;
                    }

                    float rotation[3]{
                        m_rotationDegrees.x,
                        m_rotationDegrees.y,
                        m_rotationDegrees.z
                    };

                    const auto beforeRotation = t;
                    const bool rotationChanged = ImGui::DragFloat3("Rotation", rotation, 0.5f, 0.0f, 0.0f, "%.1f deg");

                    beginEdit(beforeRotation);

                    if (rotationChanged) {
                        m_rotationDegrees = {rotation[0], rotation[1], rotation[2]};
                        t.rotation = Quat::fromEulerXYZDegrees(m_rotationDegrees).normalized();
                        m_rotationQuaternion = t.rotation;
                        changed = true;
                    }

                    endEdit();

                    float scale[3]{
                        t.scale.x, t.scale.y, t.scale.z
                    };

                    const auto beforeScale = t;
                    const bool scaleChanged = ImGui::DragFloat3("Scale", scale, 0.01f);

                    beginEdit(beforeScale);

                    if (scaleChanged) {
                        t.scale = {scale[0], scale[1], scale[2]};
                        changed = true;
                    }

                    endEdit();

                    ImGui::PopID();
                } else {
                    ImGui::TextUnformatted("No Transform component");
                }
            }
        }
        ImGui::End();
        if (m_transformEdit && !editActiveThisFrame) {
            finishTransformEdit(scene, history);
        }
        return changed;
    }

    auto InspectorPanel::finishTransformEdit(AetherEngine::scene::Scene &scene,
        commands::CommandHistory &history) -> void {
        if (!m_transformEdit) {
            return;
        }

        const auto edit = *m_transformEdit;
        m_transformEdit.reset();

        const auto entity = scene.findEntityById(edit.entityId);
        if (!entity) {
            return;
        }

        const auto* component =
            scene.tryGetComponent<AetherEngine::components::TransformComponent>(
                *entity
            );

        if (!component) {
            return;
        }

        const auto& after = component->transform;
        if (edit.before.position == after.position &&
            edit.before.rotation == after.rotation &&
            edit.before.scale == after.scale) {
            return;
        }

        history.recordApplied(std::make_unique<commands::TransformChangeCommand>(edit.entityId, edit.before, after));
    }

    auto InspectorPanel::hasPendingEdit() const -> bool {
        return m_transformEdit.has_value();
    }
} // panel
// AetherEditor