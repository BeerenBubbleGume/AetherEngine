//
// Created by drhaz on 08.10.2026.
//

#ifndef SMB_ENTITYCOMMANDS_HPP
#define SMB_ENTITYCOMMANDS_HPP

#include <string>
#include <utility>

#include "EditorCommand.hpp"
#include "scene/EntityId.hpp"

namespace AetherEditor::commands {
    class RenameEntityCommand final : public EditorCommand {
    public:
        RenameEntityCommand(AetherEngine::scene::EntityId entityId, std::string before, std::string after)
            : m_entityId(entityId), m_before(std::move(before)), m_after(std::move(after)) {}

        auto apply(AetherEngine::scene::Scene& scene) -> bool override;
        auto undo(AetherEngine::scene::Scene& scene) -> bool override;

    private:
        auto setName(AetherEngine::scene::Scene& scene, const std::string& name) const -> bool;

        AetherEngine::scene::EntityId m_entityId;
        std::string m_before;
        std::string m_after;
    };

    class CreateEntityCommand final : public EditorCommand {
    public:
        CreateEntityCommand(AetherEngine::scene::EntityId entityId, std::string name)
            : m_entityId(entityId), m_name(std::move(name)) {}

        auto apply(AetherEngine::scene::Scene& scene) -> bool override;
        auto undo(AetherEngine::scene::Scene& scene) -> bool override;

    private:
        AetherEngine::scene::EntityId m_entityId;
        std::string m_name;
    };
}

#endif //SMB_ENTITYCOMMANDS_HPP
