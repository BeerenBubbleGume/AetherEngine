//
// Created by drhaz on 30.06.2026.
//

#ifndef SMB_SYSCENESERIALIZERSYSTEM_HPP
#define SMB_SYSCENESERIALIZERSYSTEM_HPP
#include <expected>
#include <filesystem>
#include <memory>
#include <string>

#include "resources/RResourceManager.hpp"
#include "scene/SCScene.hpp"


namespace engine::systems {
    struct SYSceneSerializerError {
        int code;
        std::string message;
    };

    struct SYSceneDeserializeContext {
        resources::RResourceManager* resources{nullptr};
        std::filesystem::path assetsRoot;
    };

    class SYSceneSerializerSystem {
    public:
        struct SYSceneSerializerDeleter {
            void operator()(SYSceneSerializerSystem* ptr) const { delete ptr; }
        };
        using SYSceneSerializerPtr = std::unique_ptr<SYSceneSerializerSystem, SYSceneSerializerDeleter>;
        SYSceneSerializerSystem(const SYSceneSerializerSystem&) = default;
        SYSceneSerializerSystem& operator=(const SYSceneSerializerSystem&) = default;

        [[nodiscard]] static auto createSceneSerializerSystem(const std::filesystem::path &scenesPath) -> SYSceneSerializerPtr;
        [[nodiscard]] auto serializeScene(const scene::SCScene &scene) const -> std::expected<void, SYSceneSerializerError>;

        [[nodiscard]] auto deserializeScene(
            std::string_view sceneName,
            SYSceneDeserializeContext context = {}
        ) const -> std::expected<scene::SCScene::SScenePtr, SYSceneSerializerError>;

        ~SYSceneSerializerSystem() = default;
    private:
        SYSceneSerializerSystem(std::filesystem::path scenesPath);
        SYSceneSerializerSystem() = default;
        std::filesystem::path m_scenesPath;
    };
}


#endif //SMB_SYSCENESERIALIZERSYSTEM_HPP
