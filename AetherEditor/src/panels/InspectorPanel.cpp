//
// Created by drhaz on 08.10.2026.
//

#include "panels/InspectorPanel.hpp"

#include <algorithm>
#include <string>

#include "assets/AssetRegistry.hpp"
#include "commands/ComponentCommands.hpp"
#include "components/MaterialComponent.hpp"
#include "components/MeshComponent.hpp"
#include "components/TransformComponent.hpp"

namespace {
    auto assetDisplayName(const std::string& alias) -> const char* {
        const auto separator = alias.find_last_of('/');
        const auto offset = separator == std::string::npos ? 0 : separator + 1;
        return offset < alias.size() ? alias.c_str() + offset : alias.c_str();
    }

    auto drawAssetPicker(
        const AetherEngine::assets::AssetRegistry& registry,
        AetherEngine::assets::AssetType type,
        AetherEngine::assets::AssetId currentId,
        const char* missingLabel,
        const char* emptyLabel
    ) -> std::optional<AetherEngine::assets::AssetId> {
        const auto* current = registry.find(currentId);
        const char* preview = current && current->type == type
            ? assetDisplayName(current->alias)
            : missingLabel;

        std::optional<AetherEngine::assets::AssetId> picked;
        if (ImGui::BeginCombo("Asset", preview)) {
            auto assets = registry.listAssets();
            std::ranges::sort(assets, {}, &AetherEngine::assets::AssetRecord::alias);

            bool hasAssets = false;
            for (const auto& asset : assets) {
                if (asset.type != type) {
                    continue;
                }
                hasAssets = true;

                const auto id = AetherEngine::assets::toString(asset.id);
                ImGui::PushID(id.c_str());
                const bool selected = currentId == asset.id;
                if (ImGui::Selectable(assetDisplayName(asset.alias), selected) && !selected) {
                    picked = asset.id;
                }
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("%s", asset.alias.c_str());
                }
                if (selected) {
                    ImGui::SetItemDefaultFocus();
                }
                ImGui::PopID();
            }
            if (!hasAssets) {
                ImGui::TextDisabled("%s", emptyLabel);
            }

            ImGui::EndCombo();
        }
        return picked;
    }

    auto drawAssetId(AetherEngine::assets::AssetId assetId) -> void {
        const auto id = AetherEngine::assets::toString(assetId);
        ImGui::TextWrapped("ID: %s", id.c_str());
    }
}

namespace AetherEditor::panels {
    auto InspectorPanel::createInspectorPanel() -> InspectorPanelPtr {
        return InspectorPanelPtr(new InspectorPanel(), InspectorPanelDeleter());
    }

    auto InspectorPanel::draw(
        bool& open,
        AetherEngine::scene::Scene& scene,
        const core::EditorSession& session,
        commands::CommandHistory& history,
        const AetherEngine::assets::AssetRegistry& registry
    ) -> bool {
        bool changed = false;
        bool transformEditActive = false;
        if (ImGui::Begin("Inspector", &open)) {
            if (!session.selectedEntity) {
                ImGui::TextUnformatted("No entity selected");
            } else if (const auto entity = scene.findEntityById(*session.selectedEntity)) {
                changed = drawEntity(scene, *entity, history, registry, transformEditActive);
            } else {
                ImGui::TextUnformatted("Selected entity no longer exists");
            }
        }
        ImGui::End();

        if (m_transformEdit && !transformEditActive) {
            finishTransformEdit(scene, history);
        }
        return changed;
    }

    auto InspectorPanel::drawEntity(
        AetherEngine::scene::Scene& scene,
        AetherEngine::scene::Entity entity,
        commands::CommandHistory& history,
        const AetherEngine::assets::AssetRegistry& registry,
        bool& transformEditActive
    ) -> bool {
        const auto& identity = scene.getComponent<AetherEngine::components::IdentityComponent>(entity);
        const auto widgetId = AetherEngine::scene::toString(identity.id);
        ImGui::PushID(widgetId.c_str());
        ImGui::TextUnformatted(identity.name.c_str());
        ImGui::Separator();

        bool changed = false;
        if (auto* component = scene.tryGetComponent<AetherEngine::components::TransformComponent>(entity)) {
            changed = drawTransform(scene, identity.id, *component, history, transformEditActive) || changed;
        } else {
            ImGui::TextUnformatted("No Transform component");
        }
        changed = drawMesh(scene, entity, history, registry) || changed;
        changed = drawMaterial(scene, entity, history, registry) || changed;
        if (scene.hasComponent<AetherEngine::components::MaterialComponent>(entity) &&
            !scene.hasComponent<AetherEngine::components::MeshComponent>(entity)) {
            ImGui::TextWrapped("Assign a mesh to render this entity.");
        } else if (scene.hasComponent<AetherEngine::components::MeshComponent>(entity) &&
                   !scene.hasComponent<AetherEngine::components::MaterialComponent>(entity)) {
            ImGui::TextWrapped("Assign a material to render this entity.");
        }

        ImGui::PopID();
        return changed;
    }

    auto InspectorPanel::drawTransform(
        AetherEngine::scene::Scene& scene,
        AetherEngine::scene::EntityId entityId,
        AetherEngine::components::TransformComponent& component,
        commands::CommandHistory& history,
        bool& editActive
    ) -> bool {
        auto& t = component.transform;
        bool changed = false;
        ImGui::TextUnformatted("Transform");

        float position[3]{t.position.x, t.position.y, t.position.z};
        const auto beforePosition = t;
        const bool positionChanged = ImGui::DragFloat3("Position", position, 0.05f);
        beginTransformEdit(scene, entityId, beforePosition, history, editActive);
        if (positionChanged) {
            t.position = {position[0], position[1], position[2]};
            changed = true;
        }
        endTransformEdit(scene, history);

        if (!m_rotationEntity || *m_rotationEntity != entityId || m_rotationQuaternion != t.rotation) {
            m_rotationEntity = entityId;
            m_rotationDegrees = t.rotation.toEulerXYZDegrees();
            m_rotationQuaternion = t.rotation;
        }

        float rotation[3]{m_rotationDegrees.x, m_rotationDegrees.y, m_rotationDegrees.z};
        const auto beforeRotation = t;
        const bool rotationChanged = ImGui::DragFloat3("Rotation", rotation, 0.5f, 0.0f, 0.0f, "%.1f deg");
        beginTransformEdit(scene, entityId, beforeRotation, history, editActive);
        if (rotationChanged) {
            m_rotationDegrees = {rotation[0], rotation[1], rotation[2]};
            t.rotation = AetherEngine::math::Quat::fromEulerXYZDegrees(m_rotationDegrees).normalized();
            m_rotationQuaternion = t.rotation;
            changed = true;
        }
        endTransformEdit(scene, history);

        float scale[3]{t.scale.x, t.scale.y, t.scale.z};
        const auto beforeScale = t;
        const bool scaleChanged = ImGui::DragFloat3("Scale", scale, 0.01f);
        beginTransformEdit(scene, entityId, beforeScale, history, editActive);
        if (scaleChanged) {
            t.scale = {scale[0], scale[1], scale[2]};
            changed = true;
        }
        endTransformEdit(scene, history);
        return changed;
    }

    auto InspectorPanel::beginTransformEdit(
        AetherEngine::scene::Scene& scene,
        AetherEngine::scene::EntityId entityId,
        const AetherEngine::math::Transform& before,
        commands::CommandHistory& history,
        bool& editActive
    ) -> void {
        if (ImGui::IsItemActivated()) {
            finishTransformEdit(scene, history);
            m_transformEdit = PendingTransformEdit{entityId, ImGui::GetItemID(), before};
        }
        if (m_transformEdit && m_transformEdit->widgetId == ImGui::GetItemID() && ImGui::IsItemActive()) {
            editActive = true;
        }
    }

    auto InspectorPanel::endTransformEdit(
        AetherEngine::scene::Scene& scene,
        commands::CommandHistory& history
    ) -> void {
        if (m_transformEdit && m_transformEdit->widgetId == ImGui::GetItemID() &&
            ImGui::IsItemDeactivated() && !ImGui::IsItemActive()) {
            finishTransformEdit(scene, history);
        }
    }

    auto InspectorPanel::drawMesh(
        AetherEngine::scene::Scene& scene,
        AetherEngine::scene::Entity entity,
        commands::CommandHistory& history,
        const AetherEngine::assets::AssetRegistry& registry
    ) -> bool {
        ImGui::PushID("Mesh");
        ImGui::Separator();
        ImGui::TextUnformatted("Mesh");

        using MeshComponent = AetherEngine::components::MeshComponent;
        auto* component = scene.tryGetComponent<MeshComponent>(entity);
        bool changed = false;
        if (const auto picked = drawAssetPicker(registry, AetherEngine::assets::AssetType::Mesh,
                component ? component->asset.id : AetherEngine::assets::AssetId{},
                component ? "Unknown mesh" : "Select mesh...", "No mesh assets registered")) {
            finishTransformEdit(scene, history);
            const auto entityId = scene.getComponent<AetherEngine::components::IdentityComponent>(entity).id;
            using MeshRef = AetherEngine::assets::AssetRef<AetherEngine::assets::RuntimeMeshAsset>;
            if (component) {
                changed = history.execute(std::make_unique<commands::MeshChangeCommand>(
                    entityId, component->asset, MeshRef{*picked}
                ), scene);
            } else {
                changed = history.execute(std::make_unique<commands::AddMeshComponentCommand>(
                    entityId, MeshComponent{.asset = MeshRef{*picked}}
                ), scene);
            }
        }
        if (const auto* current = scene.tryGetComponent<MeshComponent>(entity)) {
            drawAssetId(current->asset.id);
        } else {
            ImGui::TextDisabled("Select an asset to add a Mesh component.");
        }
        ImGui::PopID();
        return changed;
    }

    auto InspectorPanel::drawMaterial(
        AetherEngine::scene::Scene& scene,
        AetherEngine::scene::Entity entity,
        commands::CommandHistory& history,
        const AetherEngine::assets::AssetRegistry& registry
    ) -> bool {
        ImGui::PushID("Material");
        ImGui::Separator();
        ImGui::TextUnformatted("Material");

        using MaterialComponent = AetherEngine::components::MaterialComponent;
        auto* component = scene.tryGetComponent<MaterialComponent>(entity);
        bool changed = false;
        if (const auto picked = drawAssetPicker(registry, AetherEngine::assets::AssetType::Material,
                component ? component->asset.id : AetherEngine::assets::AssetId{},
                component ? "Unknown material" : "Select material...", "No material assets registered")) {
            finishTransformEdit(scene, history);
            const auto entityId = scene.getComponent<AetherEngine::components::IdentityComponent>(entity).id;
            using MaterialRef = AetherEngine::assets::AssetRef<AetherEngine::assets::RuntimeMaterialAsset>;
            if (component) {
                changed = history.execute(std::make_unique<commands::MaterialChangeCommand>(
                    entityId, component->asset, MaterialRef{*picked}
                ), scene);
            } else {
                changed = history.execute(std::make_unique<commands::AddMaterialComponentCommand>(
                    entityId, MaterialComponent{.asset = MaterialRef{*picked}}
                ), scene);
            }
        }
        if (const auto* current = scene.tryGetComponent<MaterialComponent>(entity)) {
            drawAssetId(current->asset.id);
        } else {
            ImGui::TextDisabled("Select an asset to add a Material component.");
        }
        ImGui::PopID();
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

    auto InspectorPanel::hasPendingChanges(AetherEngine::scene::Scene &scene) const -> bool {
        if (!m_transformEdit) {
            return false;
        }
        const auto entity = scene.findEntityById(m_transformEdit->entityId);
        if (!entity) {
            return false;
        }
        const auto* component = scene.tryGetComponent<AetherEngine::components::TransformComponent>(*entity);
        if (!component) {
            return false;
        }
        const auto& before = m_transformEdit->before;
        const auto& current = component->transform;

        return before.position != current.position ||
                before.rotation != current.rotation ||
                before.scale != current.scale;
    }
} // panel
// AetherEditor
