//
// Created by drhaz on 08.10.2026.
//

#ifndef SMB_EDITORSESSION_HPP
#define SMB_EDITORSESSION_HPP
#include <optional>

#include "scene/EntityId.hpp"


namespace AetherEditor::core {
    class EditorSession {
    public:
        std::optional<AetherEngine::scene::EntityId> selectedEntity{};
    };
} // core
// AetherEditor

#endif //SMB_EDITORSESSION_HPP
