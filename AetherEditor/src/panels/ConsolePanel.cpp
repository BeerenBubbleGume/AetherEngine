//
// Created by drhaz on 08.10.2026.
//

#include "panels/ConsolePanel.hpp"

#include <imgui.h>


namespace AetherEditor::panels {
    auto ConsolePanel::createConsolePanel() -> ConsolePanelPtr {
        return ConsolePanelPtr(new ConsolePanel(), ConsolePanelDeleter{});
    }

    auto ConsolePanel::draw(bool &open, const AetherEngine::logging::Logger& logger) -> void {
        if (ImGui::Begin("Console", &open)) {
            ImGui::TextUnformatted("Console is ready");
            const auto record = logger.snapshot();
            if (record.empty()) {
                ImGui::TextUnformatted("No log messages");
            } else {
                for (const auto& entry : record) {
                    const auto level = AetherEngine::logging::toString(entry.level);
                    ImGui::TextDisabled(
                        "[%s] [%s]",
                        level.data(),
                        entry.category.c_str());
                    ImGui::SameLine();
                    ImGui::TextUnformatted(entry.message.data(), entry.message.data() + entry.message.size());
                }
            }
        }
        ImGui::End();
    }
} // panels
// AetherEditor