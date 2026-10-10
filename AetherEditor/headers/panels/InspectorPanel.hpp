//
// Created by drhaz on 08.10.2026.
//

#ifndef SMB_INSPECTORPANEL_HPP
#define SMB_INSPECTORPANEL_HPP
#include <memory>
#include <imgui.h>

namespace AetherEditor::panels {
    class InspectorPanel {
    public:
        struct InspectorPanelDeleter {
            void operator()(InspectorPanel* p) const {
                delete p;
            }
        };
        using InspectorPanelPtr = std::unique_ptr<InspectorPanel, InspectorPanelDeleter>;
        static auto createInspectorPanel() -> InspectorPanelPtr;
        auto draw(bool &open) -> void;
        InspectorPanel(const InspectorPanel&) = delete;
        InspectorPanel& operator=(const InspectorPanel&) = delete;
        InspectorPanel(InspectorPanel&&) = delete;
        InspectorPanel& operator=(InspectorPanel&&) = delete;
    private:
        InspectorPanel() = default;
        ~InspectorPanel() = default;
    };
} // panel
// AetherEditor

#endif //SMB_INSPECTORPANEL_HPP
