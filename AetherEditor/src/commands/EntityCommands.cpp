//
// Created by drhaz on 08.10.2026.
//

#include "commands/EntityCommands.hpp"

#include "components/TransformComponent.hpp"
#include "core/EntityName.hpp"
#include "scene/Scene.hpp"

namespace AetherEditor::commands {
    auto RenameEntityCommand::apply(AetherEngine::scene::Scene& scene) -> bool {
        if (m_before == m_after || !core::isValidEntityName(m_after)) {
            return false;
        }
        return setName(scene, m_after);
    }

    auto RenameEntityCommand::undo(AetherEngine::scene::Scene& scene) -> bool {
        return setName(scene, m_before);
    }

    auto RenameEntityCommand::setName(AetherEngine::scene::Scene& scene, const std::string& name) const -> bool {
        const auto entity = scene.findEntityById(m_entityId);
        if (!entity) {
            return false;
        }
        scene.getComponent<AetherEngine::components::IdentityComponent>(*entity).name = name;
        return true;
    }

    auto CreateEntityCommand::apply(AetherEngine::scene::Scene& scene) -> bool {
        if (!m_entityId.isValid() || scene.findEntityById(m_entityId)) {
            return false;
        }

        const auto entity = scene.createEntityWithId(m_entityId, m_name);
        try {
            scene.addComponent<AetherEngine::components::TransformComponent>(entity);
        } catch (...) {
            scene.destroyEntity(entity);
            throw;
        }
        return true;
    }

    auto CreateEntityCommand::undo(AetherEngine::scene::Scene& scene) -> bool {
        const auto entity = scene.findEntityById(m_entityId);
        if (!entity) {
            return false;
        }
        scene.destroyEntity(*entity);
        return true;
    }
} // commands
// AetherEditor
