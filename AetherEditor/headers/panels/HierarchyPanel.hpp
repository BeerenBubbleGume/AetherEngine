//
// Created by drhaz on 08.10.2026.
//

#ifndef SMB_HIERARCHYPANEL_HPP
#define SMB_HIERARCHYPANEL_HPP
#include <memory>
#include <optional>
#include <string>
#include <imgui.h>

#include "scene/Scene.hpp"
#include "core/EditorSession.hpp"

namespace AetherEditor::panels {
    struct RenameEntityRequest {
        AetherEngine::scene::EntityId entityId;
        std::string name;
    };

    struct HierarchyFrame {
        bool createEntityRequested = false;
        std::optional<RenameEntityRequest> renameEntityRequested;
    };

    class HierarchyPanel {
    public:
        struct HierarchyPanelDeleter {
            void operator()(HierarchyPanel* panel) const {
                delete panel;
            }
        };
        using HierarchyPanelPtr = std::unique_ptr<HierarchyPanel, HierarchyPanelDeleter>;
        static auto createHierarchyPanel() -> HierarchyPanelPtr;
        auto draw(bool &open, AetherEngine::scene::Scene& scene, core::EditorSession &session) -> HierarchyFrame;
        auto cancelRename() -> void;
        [[nodiscard]] auto isRenaming() const -> bool;

        HierarchyPanel(const HierarchyPanel&) = delete;
        HierarchyPanel& operator=(const HierarchyPanel&) = delete;
        HierarchyPanel(HierarchyPanel&&) = delete;
        HierarchyPanel& operator=(HierarchyPanel&&) = delete;
    private:
        auto drawEntityRow(AetherEngine::scene::Scene& scene,
                           const AetherEngine::components::IdentityComponent& identity,
                           core::EditorSession& session, HierarchyFrame& frame) -> void;
        auto drawRenameInput(HierarchyFrame& frame) -> void;
        auto finishRename(HierarchyFrame& frame) -> void;

        struct PendingRename {
            AetherEngine::scene::Scene* scene;
            AetherEngine::scene::EntityId entityId;
            std::string before;
            std::string name;
            bool focusRequested = true;
        };
        std::optional<PendingRename> m_rename;

        HierarchyPanel() = default;
        ~HierarchyPanel() = default;
    };
} // panels
// AetherEditor

#endif //SMB_HIERARCHYPANEL_HPP
