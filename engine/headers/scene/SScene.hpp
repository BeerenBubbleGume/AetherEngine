//
// Created by drhaz on 21.06.2026.
//

#ifndef SMB_SSCENE_HPP
#define SMB_SSCENE_HPP

#include <vector>
#include <array>
#include <span>

#include "graphics/GMesh.hpp"
#include "graphics/GProgram.hpp"
#include "resources/RResourceManager.hpp"

namespace engine::scene {
    struct SRenderObject {
        graphics::GTransform transform;

        engine::resources::RMeshHandle mesh;
        engine::resources::RProgramHandle program;
    };
    class SScene {
    public:
        struct SSceneDeleter {
            void operator()(SScene* scene) const {
                delete scene;
            }
        };
        using SScenePtr = std::unique_ptr<SScene, SSceneDeleter>;
        SScene(const SScene&) = delete;
        SScene& operator=(const SScene&) = delete;
        SScene() = default;
        ~SScene() = default;

        static SScenePtr createScene();
        auto addObject(SRenderObject object) -> void;
        [[nodiscard]] auto renderObjects() const -> std::span<const SRenderObject>;
    private:
        std::vector<SRenderObject> m_objects;
    };
} // scene
// engine

#endif //SMB_SSCENE_HPP
