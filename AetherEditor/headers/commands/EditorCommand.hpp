//
// Created by drhaz on 08.10.2026.
//

#ifndef SMB_EDITORCOMMAND_HPP
#define SMB_EDITORCOMMAND_HPP

namespace AetherEngine::scene {
    class Scene;
}

namespace AetherEditor::commands {
    class EditorCommand {
    public:
        virtual ~EditorCommand() = default;

        virtual auto apply(AetherEngine::scene::Scene& scene) -> bool = 0;
        virtual auto undo(AetherEngine::scene::Scene& scene) -> bool = 0;
    };
} // commands
// AetherEditor

#endif //SMB_EDITORCOMMAND_HPP
