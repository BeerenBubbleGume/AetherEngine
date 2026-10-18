//
// Created by drhaz on 08.10.2026.
//

#ifndef SMB_CONSOLEPANEL_HPP
#define SMB_CONSOLEPANEL_HPP
#include <memory>

#include "logging/Logger.hpp"


namespace AetherEditor::panels {
    class ConsolePanel {
    public:
        struct ConsolePanelDeleter {
            void operator()(ConsolePanel* panel) const {
                delete panel;
            }
        };
        using ConsolePanelPtr = std::unique_ptr<ConsolePanel, ConsolePanelDeleter>;
        static auto createConsolePanel() -> ConsolePanelPtr;
        auto draw(bool &open, const AetherEngine::logging::Logger& logger) -> void;

        ConsolePanel(const ConsolePanel&) = delete;
        ConsolePanel& operator=(const ConsolePanel&) = delete;
        ConsolePanel(ConsolePanel&&) = delete;
        ConsolePanel& operator=(ConsolePanel&&) = delete;
    private:
        ConsolePanel() = default;
        ~ConsolePanel() = default;
    };
} // panels
// AetherEditor

#endif //SMB_CONSOLEPANEL_HPP
