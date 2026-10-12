//
// Created by drhaz on 08.10.2026.
//

#include "commands/CommandHistory.hpp"


namespace AetherEditor::commands {
    auto CommandHistory::execute(std::unique_ptr<EditorCommand> command, AetherEngine::scene::Scene &scene) -> bool {
        if (!command) {
            return false;
        }
        m_commands.reserve(m_cursor + 1);
        if (!command->apply(scene)) {
            return false;
        }
        return recordApplied(std::move(command));
    }

    auto CommandHistory::recordApplied(std::unique_ptr<EditorCommand> command) -> bool {
        if (!command) {
            return false;
        }
        m_commands.reserve(m_cursor + 1);
        m_commands.resize(m_cursor);
        m_commands.push_back(std::move(command));
        m_cursor = m_commands.size();
        return true;
    }

    auto CommandHistory::undo(AetherEngine::scene::Scene &scene) -> bool {
        if (!canUndo()) {
            return false;
        }
        if  (!m_commands[m_cursor - 1]->undo(scene)) {
            return false;
        }
        --m_cursor;
        return true;
    }

    auto CommandHistory::redo(AetherEngine::scene::Scene &scene) -> bool {
        if (!canRedo()) {
            return false;
        }
        if (!m_commands[m_cursor]->apply(scene)) {
            return false;
        }
        ++m_cursor;
        return true;
    }

    auto CommandHistory::clear() -> void {
        m_commands.clear();
        m_cursor = 0;
    }

    auto CommandHistory::canUndo() const -> bool {
        return m_cursor > 0;
    }

    auto CommandHistory::canRedo() const -> bool {
        return  m_cursor < m_commands.size();
    }
} // commands
// AetherEditor