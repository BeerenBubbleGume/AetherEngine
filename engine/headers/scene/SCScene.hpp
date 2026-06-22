//
// Created by drhaz on 21.06.2026.
//

#ifndef SMB_SSCENE_HPP
#define SMB_SSCENE_HPP

#include <vector>
#include <array>
#include <span>

#include "SCCamera.hpp"
#include "graphics/GMesh.hpp"
#include "graphics/GProgram.hpp"
#include "resources/RResourceManager.hpp"

namespace engine::scene {
    using SObjectId = std::size_t;
    struct SRenderObject {
        graphics::GTransform transform;

        engine::resources::RMeshHandle mesh;
        engine::resources::RProgramHandle program;
    };
    class SCScene {
    public:
        struct SSceneDeleter {
            void operator()(SCScene* scene) const {
                delete scene;
            }
        };
        using SScenePtr = std::unique_ptr<SCScene, SSceneDeleter>;
        SCScene(const SCScene&) = delete;
        SCScene& operator=(const SCScene&) = delete;
        SCScene() = default;
        ~SCScene() = default;

        static SScenePtr createScene();
        auto addObject(SRenderObject object) -> SObjectId;
        auto addCamera(SCCamera camera) -> void;
        [[nodiscard]] auto getCamera() -> SCCamera &;
        [[nodiscard]] auto renderObjects() const -> std::span<const SRenderObject>;
        [[nodiscard]] auto getObject(SObjectId id) -> SRenderObject&;
    private:
        std::vector<SRenderObject> m_objects;
        std::unique_ptr<SCCamera> m_camera;

    };
} // scene
// engine

#endif //SMB_SSCENE_HPP
