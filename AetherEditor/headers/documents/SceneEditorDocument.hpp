//
// Created by drhaz on 08.10.2026.
//

#ifndef SMB_SCENEEDITORDOCUMENT_HPP
#define SMB_SCENEEDITORDOCUMENT_HPP

#include <string>

namespace AetherEngine::scene {
    class Scene;
}

namespace AetherEditor::docs {
    class SceneEditorDocument {
    public:
        AetherEngine::scene::Scene* scene = nullptr;
        std::string fileStem;
        bool dirty = false;
    };
} // docs
// AetherEditor

#endif //SMB_SCENEEDITORDOCUMENT_HPP
