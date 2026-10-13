//
// Created by drhaz on 08.10.2026.
//

#ifndef SMB_COMMANDHISTORY_HPP
#define SMB_COMMANDHISTORY_HPP
#include <memory>
#include "scene/Scene.hpp"
#include "EditorCommand.hpp"

namespace AetherEditor::commands {
    class CommandHistory {
    public:
        auto execute(std::unique_ptr<EditorCommand> command, AetherEngine::scene::Scene& scene) -> bool;
        auto recordApplied(std::unique_ptr<EditorCommand> command) -> bool;
        auto undo(AetherEngine::scene::Scene& scene) -> bool;
        auto redo(AetherEngine::scene::Scene& scene) -> bool;
        auto clear() -> void;
        auto markSaved() -> void;

        [[nodiscard]] auto canUndo() const -> bool;
        [[nodiscard]] auto canRedo() const -> bool;
        [[nodiscard]] auto isDirty() const -> bool;
    private:
        std::vector<std::unique_ptr<EditorCommand>> m_commands;
        std::size_t m_cursor = 0;
        std::optional<std::size_t> m_savedCursor{0};
    };
} // commands
// AetherEditor

#endif //SMB_COMMANDHISTORY_HPP
