//
// Created by drhaz on 08.10.2026.
//

#include "commands/EntityCommands.hpp"

#include <algorithm>

#include "components/TransformComponent.hpp"
#include "core/EntityName.hpp"
#include "scene/Scene.hpp"

namespace {
    auto duplicateName(AetherEngine::scene::Scene& scene, const std::string& sourceName) -> std::string {
        const std::string base = AetherEditor::core::isValidEntityName(sourceName) ? sourceName : "Entity";
        for (std::size_t index = 0;; ++index) {
            const auto suffix = index == 0 ? std::string{" Copy"} : " Copy (" + std::to_string(index) + ")";
            auto length = std::min(base.size(), AetherEditor::core::MaxEntityNameBytes - suffix.size());
            // Truncate at a UTF-8 boundary so the generated name still round-trips through JSON.
            while (length > 0 && length < base.size() &&
                   (static_cast<unsigned char>(base[length]) & 0xc0U) == 0x80U) {
                --length;
            }
            auto name = base.substr(0, length) + suffix;
            if (!scene.findEntityByName(name)) {
                return name;
            }
        }
    }
}

namespace AetherEditor::commands {
    auto DeleteEntityCommand::apply(AetherEngine::scene::Scene& scene) -> bool {
        const auto entity = scene.findEntityById(m_entityId);
        if (!entity) {
            return false;
        }
        if (!m_snapshot) {
            m_snapshot = captureEntity(scene, m_entityId);
        }
        scene.destroyEntity(*entity);
        return true;
    }

    auto DeleteEntityCommand::undo(AetherEngine::scene::Scene& scene) -> bool {
        return m_snapshot && restoreEntity(scene, *m_snapshot);
    }

    auto DuplicateEntityCommand::apply(AetherEngine::scene::Scene& scene) -> bool {
        if (!m_duplicateId.isValid() || m_duplicateId == m_sourceId || scene.findEntityById(m_duplicateId)) {
            return false;
        }
        if (!m_snapshot) {
            auto snapshot = captureEntity(scene, m_sourceId);
            if (!snapshot) {
                return false;
            }
            snapshot->id = m_duplicateId;
            snapshot->name = duplicateName(scene, snapshot->name);
            snapshot->wasActiveCamera = false;
            m_snapshot = std::move(snapshot);
        }
        return restoreEntity(scene, *m_snapshot);
    }

    auto DuplicateEntityCommand::undo(AetherEngine::scene::Scene& scene) -> bool {
        const auto entity = scene.findEntityById(m_duplicateId);
        if (!entity || !m_snapshot) {
            return false;
        }
        scene.destroyEntity(*entity);
        return true;
    }

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
