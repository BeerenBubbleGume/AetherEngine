#include "core/AetherEditor.hpp"
#include "core/AetherEngine.hpp"

int main() {
    auto engine = AetherEngine::Engine::createEngine();
    if (!engine) {
        return 1;
    }

    auto initResult = engine->initEngine();
    if (!initResult) {
        return 1;
    }

    auto editor = AetherEditor::core::Editor::createEditor();
    auto runResult = engine->run(*editor);
    return runResult ? 0 : 1;
}
