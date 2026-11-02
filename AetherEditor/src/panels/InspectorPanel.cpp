//
// Created by drhaz on 08.10.2026.
//

#include "panels/InspectorPanel.hpp"

#include <algorithm>
#include <limits>
#include <string>

#include "assets/AssetRegistry.hpp"
#include "commands/ComponentCommands.hpp"
#include "components/MaterialComponent.hpp"
#include "components/MeshComponent.hpp"
#include "components/TransformComponent.hpp"
#include "physics/PhysicsValidation.hpp"

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
        const AetherEngine::assets::AssetRegistry& registry,
        std::span<const AetherEngine::systems::SceneAssetBindingError> errors
    ) -> bool {
        bool changed = false;
        bool editActive = false;
        if ((m_transformEdit && session.selectedEntity != m_transformEdit->entityId) ||
            (m_physicsEdit && session.selectedEntity != m_physicsEdit->entityId)) {
            finishPendingEdits(scene, history);
        }
        if (ImGui::Begin("Inspector", &open)) {
            if (!session.selectedEntity) {
                ImGui::TextUnformatted("No entity selected");
            } else if (const auto entity = scene.findEntityById(*session.selectedEntity)) {
                changed = drawEntity(scene, *entity, history, registry, editActive);
                const auto* mesh = scene.tryGetComponent<AetherEngine::components::MeshComponent>(*entity);
                const auto* material = scene.tryGetComponent<AetherEngine::components::MaterialComponent>(*entity);
                for (const auto& error : errors) {
                    if ((mesh && mesh->asset.id == error.asset) || (material && material->asset.id == error.asset)) {
                        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{1.0f, 0.45f, 0.4f, 1.0f});
                        ImGui::TextWrapped("%s", error.message.c_str());
                        ImGui::PopStyleColor();
                        ImGui::TextWrapped("Rendering is skipped until the asset is available. Choose another asset or use View > Retry Missing Assets.");
                    }
                }
            } else {
                ImGui::TextUnformatted("Selected entity no longer exists");
            }
        }
        ImGui::End();

        if (!editActive) {
            finishPendingEdits(scene, history);
        }
        return changed;
    }

    auto InspectorPanel::drawEntity(
        AetherEngine::scene::Scene& scene,
        AetherEngine::scene::Entity entity,
        commands::CommandHistory& history,
        const AetherEngine::assets::AssetRegistry& registry,
        bool& editActive
    ) -> bool {
        const auto& identity = scene.getComponent<AetherEngine::components::IdentityComponent>(entity);
        const auto widgetId = AetherEngine::scene::toString(identity.id);
        ImGui::PushID(widgetId.c_str());
        ImGui::TextUnformatted(identity.name.c_str());
        ImGui::Separator();

        bool changed = false;
        if (auto* component = scene.tryGetComponent<AetherEngine::components::TransformComponent>(entity)) {
            changed = drawTransform(scene, identity.id, *component, history, editActive) || changed;
        } else {
            ImGui::TextUnformatted("No Transform component");
        }
        changed = drawPhysicsActions(scene, entity, history) || changed;
        if (auto* component = scene.tryGetComponent<AetherEngine::components::RigidbodyComponent>(entity)) {
            changed = drawRigidbody(scene, identity.id, *component, history, editActive) || changed;
        }
        if (auto* component = scene.tryGetComponent<AetherEngine::components::ColliderComponent>(entity)) {
            changed = drawCollider(scene, identity.id, *component, history, editActive) || changed;
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

    auto InspectorPanel::drawRigidbody(AetherEngine::scene::Scene &scene, AetherEngine::scene::EntityId entityId,
        AetherEngine::components::RigidbodyComponent &component, commands::CommandHistory &history,
        bool &editActive) -> bool {
        bool changed = false;
        ImGui::PushID("Rigidbody");
        ImGui::Separator();
        ImGui::TextUnformatted("Rigidbody");
        const auto entity = *scene.findEntityById(entityId);
        auto before = commands::PhysicsComponentState::capture(scene, entity);
        int bodyType = component.dynamic ? 1 : 0;
        if (ImGui::Combo("Body Type", &bodyType, "Static\0Dynamic\0")) {
            finishPendingEdits(scene, history);
            auto after = before;
            after.rigidbody->dynamic = bodyType == 1;
            changed = history.execute(std::make_unique<commands::PhysicsChangeCommand>(entityId, before, after), scene) || changed;
        }

        ImGui::BeginDisabled(!component.dynamic);
        before = commands::PhysicsComponentState::capture(scene, entity);
        float mass = component.mass;
        const bool massChanged = ImGui::DragFloat("Mass", &mass, 0.01f, std::numeric_limits<float>::min(),
            AetherEngine::physics::MaxBodyMass, "%.3f", ImGuiSliderFlags_AlwaysClamp);
        beginPhysicsEdit(scene, entityId, before, history, editActive);
        if (massChanged && AetherEngine::physics::validPositivePhysicsValue(mass, AetherEngine::physics::MaxBodyMass)) {
            component.mass = mass;
            changed = true;
        }
        endPhysicsEdit(scene, history);

        before = commands::PhysicsComponentState::capture(scene, entity);
        bool useGravity = component.useGravity;
        if (ImGui::Checkbox("Use Gravity", &useGravity)) {
            finishPendingEdits(scene, history);
            auto after = before;
            after.rigidbody->useGravity = useGravity;
            changed = history.execute(std::make_unique<commands::PhysicsChangeCommand>(entityId, before, after), scene) || changed;
        }
        ImGui::EndDisabled();
        ImGui::PopID();
        return changed;
    }

    auto InspectorPanel::drawPhysicsActions(AetherEngine::scene::Scene& scene, AetherEngine::scene::Entity entity,
                                            commands::CommandHistory& history) -> bool {
        using namespace AetherEngine::components;
        ImGui::PushID("Physics");
        ImGui::Separator();
        const auto before = commands::PhysicsComponentState::capture(scene, entity);
        const auto id = scene.getComponent<IdentityComponent>(entity).id;
        const bool hasTransform = scene.hasComponent<TransformComponent>(entity);
        bool physicsEnabled = before.rigidbody.has_value() && before.collider.has_value();
        const bool incomplete = before.rigidbody.has_value() != before.collider.has_value();
        bool changed = false;
        // A checked box means both authoring components exist. Removing them is
        // still allowed when an externally authored entity has no Transform.
        ImGui::BeginDisabled(!hasTransform && !physicsEnabled);
        if (ImGui::Checkbox("Physics", &physicsEnabled)) {
            finishPendingEdits(scene, history);
            auto after = before;
            if (physicsEnabled) {
                if (!after.rigidbody) after.rigidbody = RigidbodyComponent{.dynamic = false};
                if (!after.collider) after.collider = ColliderComponent{};
            } else {
                after = {};
            }
            changed = history.execute(std::make_unique<commands::PhysicsChangeCommand>(id, before, after), scene);
        }
        ImGui::EndDisabled();
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
            ImGui::SetTooltip("Add or remove Rigidbody and Collider. Undo restores removed settings.");
        }
        if (!hasTransform) ImGui::TextWrapped("Physics is inactive: a Transform component is required.");
        if (incomplete && !changed) {
            ImGui::TextWrapped("Physics is incomplete. Enable Physics to add the missing component and keep existing settings.");
            if (ImGui::Button("Remove incomplete physics")) {
                finishPendingEdits(scene, history);
                changed = history.execute(std::make_unique<commands::PhysicsChangeCommand>(id, before, commands::PhysicsComponentState{}), scene);
            }
        }
        ImGui::PopID();
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
            finishPendingEdits(scene, history);
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

    auto InspectorPanel::beginPhysicsEdit(AetherEngine::scene::Scene &scene, AetherEngine::scene::EntityId entityId,
        const commands::PhysicsComponentState &before, commands::CommandHistory &history, bool &editActive) -> void {
        if (ImGui::IsItemActivated()) {
            finishPendingEdits(scene, history);
            m_physicsEdit = PendingPhysicsEdit{entityId, ImGui::GetItemID(), before};
        }
        if (m_physicsEdit && m_physicsEdit->widgetId == ImGui::GetItemID() && ImGui::IsItemActive()) {
            editActive = true;
        }
    }

    auto InspectorPanel::endPhysicsEdit(AetherEngine::scene::Scene &scene, commands::CommandHistory &history) -> void {
        if (m_physicsEdit && m_physicsEdit->widgetId == ImGui::GetItemID() && ImGui::IsKeyPressed(ImGuiKey_Escape)) {
            const auto edit = *m_physicsEdit;
            m_physicsEdit.reset();
            if (const auto entity = scene.findEntityById(edit.entityId)) {
                commands::PhysicsChangeCommand cancel(edit.entityId, commands::PhysicsComponentState::capture(scene, *entity), edit.before);
                cancel.apply(scene);
            }
        }
        if (m_physicsEdit && m_physicsEdit->widgetId == ImGui::GetItemID() &&
            ImGui::IsItemDeactivated() && !ImGui::IsItemActive()) {
            finishPhysicsEdit(scene, history);
        }
    }

    auto InspectorPanel::drawCollider(AetherEngine::scene::Scene &scene, AetherEngine::scene::EntityId entityId,
                                      AetherEngine::components::ColliderComponent &component, commands::CommandHistory &history,
                                      bool& editActive) -> bool {
        using namespace AetherEngine::physics;
        using AetherEngine::components::ColliderType;
        const auto entity = *scene.findEntityById(entityId);
        bool changed = false;
        ImGui::PushID("Collider");
        ImGui::Separator();
        ImGui::TextUnformatted("Collider");

        auto before = commands::PhysicsComponentState::capture(scene, entity);
        int shape = static_cast<int>(component.type);
        if (ImGui::Combo("Shape", &shape, "Box\0Sphere\0Capsule\0")) {
            finishPendingEdits(scene, history);
            auto after = before;
            after.collider->type = static_cast<ColliderType>(shape);
            changed = history.execute(std::make_unique<commands::PhysicsChangeCommand>(entityId, before, after), scene) || changed;
        }

        const auto drawDimension = [&](const char* label, float& value) {
            const auto previous = commands::PhysicsComponentState::capture(scene, entity);
            float edited = value;
            const bool editedValue = ImGui::DragFloat(label, &edited, 0.01f, std::numeric_limits<float>::min(),
                MaxColliderDimension, "%.3f", ImGuiSliderFlags_AlwaysClamp);
            beginPhysicsEdit(scene, entityId, previous, history, editActive);
            if (editedValue && validPositivePhysicsValue(edited, MaxColliderDimension)) {
                value = edited;
                changed = true;
            }
            endPhysicsEdit(scene, history);
        };
        if (component.type == ColliderType::Box) {
            before = commands::PhysicsComponentState::capture(scene, entity);
            float dimensions[3]{component.size.x * 2.0f, component.size.y * 2.0f, component.size.z * 2.0f};
            const bool dimensionsChanged = ImGui::DragFloat3("Dimensions", dimensions, 0.01f,
                std::numeric_limits<float>::min() * 2.0f, MaxColliderDimension * 2.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
            beginPhysicsEdit(scene, entityId, before, history, editActive);
            auto candidate = component;
            candidate.size = {dimensions[0] * 0.5f, dimensions[1] * 0.5f, dimensions[2] * 0.5f};
            if (dimensionsChanged && invalidPhysicsField(candidate).empty()) {
                component = candidate;
                changed = true;
            }
            endPhysicsEdit(scene, history);
        } else {
            drawDimension("Radius", component.radius);
            if (component.type == ColliderType::Capsule) {
                drawDimension("Cylinder Height", component.height);
                ImGui::Text("Total Height: %.3f", component.height + 2.0f * component.radius);
                ImGui::TextUnformatted("Capsule axis: local X");
            }
        }
        ImGui::TextWrapped("Collider dimensions are independent of Transform Scale.");

        before = commands::PhysicsComponentState::capture(scene, entity);
        bool trigger = component.trigger;
        if (ImGui::Checkbox("Sensor / Trigger", &trigger)) {
            finishPendingEdits(scene, history);
            auto after = before;
            after.collider->trigger = trigger;
            changed = history.execute(std::make_unique<commands::PhysicsChangeCommand>(entityId, before, after), scene) || changed;
        }
        if (component.trigger) ImGui::TextWrapped("No collision response. Trigger events are not available yet.");
        ImGui::PopID();
        return changed;
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
            finishPendingEdits(scene, history);
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
            finishPendingEdits(scene, history);
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

    auto InspectorPanel::finishPhysicsEdit(AetherEngine::scene::Scene &scene,
        commands::CommandHistory &history) -> void {
        if (!m_physicsEdit) return;
        const auto edit = *m_physicsEdit;
        m_physicsEdit.reset();
        const auto entity = scene.findEntityById(edit.entityId);
        if (!entity) return;
        const auto after = commands::PhysicsComponentState::capture(scene, *entity);
        if (edit.before != after) {
            history.recordApplied(std::make_unique<commands::PhysicsChangeCommand>(edit.entityId, edit.before, after));
        }
    }

    auto InspectorPanel::finishPendingEdits(AetherEngine::scene::Scene& scene, commands::CommandHistory& history) -> void {
        finishTransformEdit(scene, history);
        finishPhysicsEdit(scene, history);
    }

    auto InspectorPanel::hasPendingEdit() const -> bool {
        return m_transformEdit.has_value() || m_physicsEdit.has_value();
    }

    auto InspectorPanel::hasPendingChanges(AetherEngine::scene::Scene &scene) const -> bool {
        if (m_transformEdit) {
            if (const auto entity = scene.findEntityById(m_transformEdit->entityId)) {
                if (const auto* component = scene.tryGetComponent<AetherEngine::components::TransformComponent>(*entity)) {
                    const auto& before = m_transformEdit->before;
                    const auto& current = component->transform;
                    if (before.position != current.position || before.rotation != current.rotation || before.scale != current.scale)
                        return true;
                }
            }
        }
        if (m_physicsEdit) {
            if (const auto entity = scene.findEntityById(m_physicsEdit->entityId)) {
                return m_physicsEdit->before != commands::PhysicsComponentState::capture(scene, *entity);
            }
        }
        return false;
    }
} // panel
// AetherEditor
