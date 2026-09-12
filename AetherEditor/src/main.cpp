#include "core/AetherEditor.hpp"
#include "core/AetherEngine.hpp"

#include <iostream>
#include <string_view>

int main(int argc, char* argv[]) {
    AetherEngine::core::EngineInitConfig config{
        .projectRoot = AETHER_DEFAULT_PROJECT_ROOT
    };
    for (int i = 1; i < argc; ++i) {
        const std::string_view argument{argv[i]};
        if (argument == "--help") {
            std::cout << "Usage: AetherEditor [--project <directory>] [--scene <name>]\n"
                      << "Default project: " << config.projectRoot.string() << '\n';
            return 0;
        }
        if ((argument != "--project" && argument != "--scene") ||
            i + 1 >= argc || std::string_view{argv[i + 1]}.empty() ||
            std::string_view{argv[i + 1]}.starts_with("--")) {
            std::cerr << "Invalid argument: " << argument << ". Use --help for usage.\n";
            return 1;
        }
        if (argument == "--project") {
            config.projectRoot = argv[++i];
        } else {
            config.startupScene = argv[++i];
        }
    }
    auto engine = AetherEngine::Engine::createEngine();
    if (!engine) {
        return 1;
    }

    auto initResult = engine->initEngine(config);
    if (!initResult) {
        std::cerr << initResult.error().message << '\n';
        return 1;
    }

    std::cout << "Opened project: " << config.projectRoot.string() << '\n';

    auto editor = AetherEditor::core::Editor::createEditor();
    auto runResult = engine->run(*editor);
    return runResult ? 0 : 1;
}
