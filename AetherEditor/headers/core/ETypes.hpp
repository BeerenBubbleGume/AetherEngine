//
// Created by drhaz on 15.07.2026.
//

#ifndef SMB_ETYPES_HPP
#define SMB_ETYPES_HPP
#include <string>
#include <scene/Scene.hpp>

namespace AetherEditor::core {
    struct EditorError {
        int code;
        std::string msg;
    };
    enum class DocumentActionKind {
        NewScene,
        OpenScene
    };
    struct PendingDocumentAction {
        DocumentActionKind kind;
        std::string sceneName;
    };
    enum class EditorWorkspacePanels {
        AssetBrowser,
        Console,
        Hierarchy,
        Inspector,
        SceneViewport
    };
    enum class SceneLoadDecision {
        None,
        DiscardChanges,
        Cancel
    };
}


#endif //SMB_ETYPES_HPP
