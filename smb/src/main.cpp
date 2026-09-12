//
// Created by drhaz on 14.06.2026.
//

#include "core/AetherEngine.hpp"
#include "core/Smb.hpp"

int main() {
    auto engine = AetherEngine::Engine::createEngine();
    if (!engine) {
        return 1;
    }

    auto initResult = engine->initEngine();
    if (!initResult && engine->getState() != AetherEngine::core::EngineState::Initialized) {
        std::cerr << initResult.error().message << '\n';
        return 1;
    }

    auto app = smb::SMB::createSMB();

    auto runResult = engine->run(*app);
    if (!runResult) {
        return 1;
    }

    return 0;
}
