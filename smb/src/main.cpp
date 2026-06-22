//
// Created by drhaz on 14.06.2026.
//

#include "core/Engine.hpp"
#include "core/Smb.hpp"

int main() {
    auto engine = engine::Engine::createEngine();
    if (!engine) {
        return 1;
    }

    auto initResult = engine->initEngine();
    if (!initResult) {
        return 1;
    }

    auto app = smb::SMB::createSMB();

    auto runResult = engine->run(*app);
    if (!runResult) {
        return 1;
    }

    return 0;
}
