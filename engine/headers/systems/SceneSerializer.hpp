//
// Created by drhaz on 30.06.2026.
//

#ifndef SMB_SCENESERIALIZER_HPP
#define SMB_SCENESERIALIZER_HPP
#include <expected>
#include <filesystem>
#include <memory>
#include <string>

#include "resources/ResourceManager.hpp"
#include "scene/Scene.hpp"


namespace engine::systems {
    struct SceneSerializerError {
        int code;
        std::string message;
    };

    struct SceneDeserializeContext {
        resources::ResourceManager* resources{nullptr};
        std::filesystem::path assetsRoot;
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
            std::string_view sceneName,
            SceneDeserializeContext context = {}
        ) const -> std::expected<scene::Scene::ScenePtr, SceneSerializerError>;

        ~SceneSerializer() = default;
    private:
        SceneSerializer(std::filesystem::path scenesPath);
        SceneSerializer() = default;
        std::filesystem::path m_scenesPath;
    };
}


#endif //SMB_SCENESERIALIZER_HPP
