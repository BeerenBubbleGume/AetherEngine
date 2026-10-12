//
// Created by drhaz on 08.10.2026.
//

#ifndef SMB_COMPONENTCOMMANDS_HPP
#define SMB_COMPONENTCOMMANDS_HPP
#include <utility>

#include "EditorCommand.hpp"
#include "math/Types.hpp"
#include "scene/EntityId.hpp"

namespace AetherEditor::commands {
    class TransformChangeCommand final : public EditorCommand {
    public:
        TransformChangeCommand(AetherEngine::scene::EntityId entityId, AetherEngine::math::Transform before, AetherEngine::math::Transform after)
            : m_entityId(entityId),
              m_before(std::move(before)),
              m_after(std::move(after)) {}

        auto apply(AetherEngine::scene::Scene& scene) -> bool override;
        auto undo(AetherEngine::scene::Scene& scene) -> bool override;

    private:
        auto setTransform(
            AetherEngine::scene::Scene& scene,
            const AetherEngine::math::Transform& value
        ) const -> bool;

        AetherEngine::scene::EntityId m_entityId;
        AetherEngine::math::Transform m_before;
        AetherEngine::math::Transform m_after;
    };
} // commands
// AetherEditor

#endif //SMB_COMPONENTCOMMANDS_HPP
