//
// Created by drhaz on 30.06.2026.
//

#ifndef SMB_SCENESERIALIZER_HPP
#define SMB_SCENESERIALIZER_HPP
#include <expected>
#include <filesystem>
#include <memory>
#include <string>

#include "assets/AssetManager.hpp"
#include "scene/Scene.hpp"


namespace AetherEngine::systems {
    struct SceneSerializerError {
        int code;
        std::string message;
    };

    class SceneSerializer {
    public:
        struct SceneSerializerDeleter {
            void operator()(SceneSerializer* ptr) const { delete ptr; }
        };
        using SceneSerializerPtr = std::unique_ptr<SceneSerializer, SceneSerializerDeleter>;
        SceneSerializer(const SceneSerializer&) = default;
        SceneSerializer& operator=(const SceneSerializer&) = default;

        [[nodiscard]] static auto createSceneSerializer(const std::filesystem::path &scenesPath) -> SceneSerializerPtr;
        [[nodiscard]] auto serializeScene(const scene::Scene &scene) const -> std::expected<void, SceneSerializerError>;

        [[nodiscard]] auto deserializeScene(
            std::string_view sceneName
        ) const -> std::expected<scene::Scene::ScenePtr, SceneSerializerError>;

        ~SceneSerializer() = default;
    private:
        SceneSerializer(std::filesystem::path scenesPath);
        SceneSerializer() = default;
        std::filesystem::path m_scenesPath;
    };
}


#endif //SMB_SCENESERIALIZER_HPP
