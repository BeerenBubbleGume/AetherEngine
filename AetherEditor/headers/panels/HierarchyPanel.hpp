//
// Created by drhaz on 08.10.2026.
//

#ifndef SMB_HIERARCHYPANEL_HPP
#define SMB_HIERARCHYPANEL_HPP
#include <memory>
#include <imgui.h>

namespace AetherEditor::panels {
    class HierarchyPanel {
    public:
        struct HierarchyPanelDeleter {
            void operator()(HierarchyPanel* panel) const {
                delete panel;
            }
        };
        using HierarchyPanelPtr = std::unique_ptr<HierarchyPanel, HierarchyPanelDeleter>;
        static auto createHierarchyPanel() -> HierarchyPanelPtr;
        auto draw(bool &open) -> void;

        HierarchyPanel(const HierarchyPanel&) = delete;
        HierarchyPanel& operator=(const HierarchyPanel&) = delete;
        HierarchyPanel(HierarchyPanel&&) = delete;
        HierarchyPanel& operator=(HierarchyPanel&&) = delete;
    private:
        HierarchyPanel() = default;
        ~HierarchyPanel() = default;
    };
} // panels
// AetherEditor

#endif //SMB_HIERARCHYPANEL_HPP
