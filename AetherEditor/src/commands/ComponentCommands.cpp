//
// Created by drhaz on 08.10.2026.
//

#include "commands/ComponentCommands.hpp"

#include "components/TransformComponent.hpp"
#include "scene/Scene.hpp"


namespace AetherEditor::commands {
    auto TransformChangeCommand::apply(AetherEngine::scene::Scene &scene) -> bool {
        return setTransform(scene, m_after);
    }

    auto TransformChangeCommand::undo(AetherEngine::scene::Scene &scene) -> bool {
        return setTransform(scene, m_before);
    }

    auto TransformChangeCommand::setTransform(AetherEngine::scene::Scene &scene,
        const AetherEngine::math::Transform &value) const -> bool {
        const auto entity = scene.findEntityById(m_entityId);

        if (!entity) {
            return false;
        }

        auto* component =
            scene.tryGetComponent<AetherEngine::components::TransformComponent>(
                *entity
            );

        if (!component) {
            return false;
        }

        component->transform = value;
        return true;
    }
} // commands
// AetherEditor