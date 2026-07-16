//
// Created by drhaz on 30.06.2026.
//

#include "systems/SySceneSerializerSystem.hpp"

#include <fstream>
#include <optional>
#include <utility>

#include <entt/entity/entity.hpp>
#include <nlohmann/json.hpp>

#include "components/ICameraComponent.hpp"
#include "components/IIdentityComponent.hpp"
#include "components/IMaterialComponent.hpp"
#include "components/IMeshComponent.hpp"
#include "components/IPhysicsComponents.hpp"
#include "components/ITransformComponent.hpp"

namespace {
    using Json = nlohmann::json;

    constexpr int FileSystemErrorCode = 1;
    constexpr int ParseErrorCode = 2;
    constexpr int InvalidFormatErrorCode = 3;

    auto makeError(int code, std::string message) -> engine::systems::SYSceneSerializerError {
        return {code, std::move(message)};
    }

    auto sceneFilename(const std::filesystem::path& scenesPath, std::string_view sceneName) -> std::filesystem::path {
        return scenesPath / (std::string{sceneName} + ".scene.json");
    }

    auto resolveAssetPath(const std::filesystem::path& assetsRoot, std::string_view assetPath) -> std::filesystem::path {
        std::filesystem::path path{assetPath};
        if (path.empty() || path.is_absolute()) {
            return path;
        }
        return assetsRoot / path;
    }

    auto serializeVec3(const engine::math::TVec3& value) -> Json {
        return Json::array({value.x, value.y, value.z});
    }

    auto deserializeVec3(const Json& value, engine::math::TVec3 fallback) -> engine::math::TVec3 {
        if (!value.is_array() || value.size() != 3) {
            return fallback;
        }
        return {
            value.at(0).get<float>(),
            value.at(1).get<float>(),
            value.at(2).get<float>()
        };
    }

    auto serializeQuat(const engine::math::TQuat& value) -> Json {
        return Json::array({value.w, value.x, value.y, value.z});
    }

    auto deserializeQuat(const Json& value, engine::math::TQuat fallback) -> engine::math::TQuat {
        if (!value.is_array() || value.size() != 4) {
            return fallback;
        }
        return {
            value.at(0).get<float>(),
            value.at(1).get<float>(),
            value.at(2).get<float>(),
            value.at(3).get<float>()
        };
    }

    auto serializeColor(const engine::math::TColor& value) -> Json {
        return Json::array({value.r, value.g, value.b, value.a});
    }

    auto deserializeColor(const Json& value, engine::math::TColor fallback) -> engine::math::TColor {
        if (!value.is_array() || value.size() != 4) {
            return fallback;
        }
        return {
            value.at(0).get<float>(),
            value.at(1).get<float>(),
            value.at(2).get<float>(),
            value.at(3).get<float>()
        };
    }

    auto serializeTransform(const engine::components::ITransformComponent& component) -> Json {
        return Json{
            {"position", serializeVec3(component.transform.position)},
            {"rotation", serializeQuat(component.transform.rotation)},
            {"scale", serializeVec3(component.transform.scale)}
        };
    }

    auto deserializeTransform(const Json& value) -> engine::components::ITransformComponent {
        engine::components::ITransformComponent component{};
        component.transform.position = deserializeVec3(value.value("position", Json::array()), engine::math::TVec3::zero());
        component.transform.rotation = deserializeQuat(value.value("rotation", Json::array()), engine::math::TQuat::identity());
        component.transform.scale = deserializeVec3(value.value("scale", Json::array()), engine::math::TVec3::one());
        return component;
    }

    auto serializeCamera(const engine::components::ICameraComponent& component) -> Json {
        return Json{
            {"enabled", component.enabled},
            {"projection", static_cast<int>(component.projection)},
            {"fovYDegrees", component.fovYDegrees},
            {"nearPlane", component.nearPlane},
            {"farPlane", component.farPlane},
            {"orthographicHeight", component.orthographicHeight},
            {"clearFlags", component.clearFlags},
            {"clearColor", component.clearColor},
            {"priority", component.priority},
            {"layerMask", component.layerMask}
        };
    }

    auto deserializeCamera(const Json& value) -> engine::components::ICameraComponent {
        engine::components::ICameraComponent component{};
        component.enabled = value.value("enabled", component.enabled);
        component.projection = static_cast<engine::components::ProjectionType>(
            value.value("projection", static_cast<int>(component.projection))
        );
        component.fovYDegrees = value.value("fovYDegrees", component.fovYDegrees);
        component.nearPlane = value.value("nearPlane", component.nearPlane);
        component.farPlane = value.value("farPlane", component.farPlane);
        component.orthographicHeight = value.value("orthographicHeight", component.orthographicHeight);
        component.clearFlags = value.value("clearFlags", component.clearFlags);
        component.clearColor = value.value("clearColor", component.clearColor);
        component.priority = value.value("priority", component.priority);
        component.layerMask = value.value("layerMask", component.layerMask);
        return component;
    }

    auto serializeRenderState(const engine::resources::RRenderState& value) -> Json {
        return Json{
            {"writeRgb", value.writeRgb},
            {"writeAlpha", value.writeAlpha},
            {"writeDepth", value.writeDepth},
            {"depthTest", value.depthTest},
            {"cullBackFaces", value.cullBackFaces},
            {"alphaBlend", value.alphaBlend},
            {"msaa", value.msaa}
        };
    }

    auto serializeTexturePaths(const std::vector<std::string>& texturePaths) -> Json {
        Json jsonTextures = Json::array();
        for (const auto& texturePath : texturePaths) {
            jsonTextures.push_back(texturePath);
        }
        return jsonTextures;
    }

    auto deserializeTexturePaths(const Json& value) -> std::vector<std::string> {
        std::vector<std::string> texturePaths;

        if (value.is_string()) {
            texturePaths.push_back(value.get<std::string>());
            return texturePaths;
        }

        if (!value.is_array()) {
            return texturePaths;
        }

        for (const auto& texturePath : value) {
            if (texturePath.is_string()) {
                texturePaths.push_back(texturePath.get<std::string>());
            }
        }

        return texturePaths;
    }

    auto deserializeRenderState(const Json& value) -> engine::resources::RRenderState {
        engine::resources::RRenderState state{};
        state.writeRgb = value.value("writeRgb", state.writeRgb);
        state.writeAlpha = value.value("writeAlpha", state.writeAlpha);
        state.writeDepth = value.value("writeDepth", state.writeDepth);
        state.depthTest = value.value("depthTest", state.depthTest);
        state.cullBackFaces = value.value("cullBackFaces", state.cullBackFaces);
        state.alphaBlend = value.value("alphaBlend", state.alphaBlend);
        state.msaa = value.value("msaa", state.msaa);
        return state;
    }

    auto serializeMesh(const engine::components::IMeshComponent& component) -> Json {
        return Json{
            {"assetPath", component.assetPath}
        };
    }

    auto deserializeMesh(
        const Json& value,
        const engine::systems::SYSceneDeserializeContext& context
    ) -> engine::components::IMeshComponent {
        const auto assetPath = value.value("assetPath", std::string{});
        engine::resources::RMeshHandle mesh{};
        if (context.resources && !assetPath.empty()) {
            mesh = context.resources->loadMesh(resolveAssetPath(context.assetsRoot, assetPath).string());
        }
        return {
            .mesh = mesh,
            .assetPath = assetPath
        };
    }

    auto serializeMaterial(const engine::components::IMaterialComponent& component) -> Json {
        return Json{
            {"programName", component.programName},
            {"vertexShaderPath", component.vertexShaderPath},
            {"fragmentShaderPath", component.fragmentShaderPath},
            {"baseColor", serializeColor(component.material.baseColor)},
            {"renderState", serializeRenderState(component.material.renderState)},
            {"textures", serializeTexturePaths(component.texturePaths)}
        };
    }

    auto deserializeMaterial(
        const Json& value,
        const engine::systems::SYSceneDeserializeContext& context
    ) -> engine::components::IMaterialComponent {
        const auto programName = value.value("programName", std::string{});
        const auto vertexShaderPath = value.value("vertexShaderPath", std::string{});
        const auto fragmentShaderPath = value.value("fragmentShaderPath", std::string{});
        std::vector<std::string> texturePaths;
        if (value.contains("textures")) {
            texturePaths = deserializeTexturePaths(value.at("textures"));
        } else if (value.contains("texturePaths")) {
            texturePaths = deserializeTexturePaths(value.at("texturePaths"));
        }

        engine::resources::RProgramHandle program{};
        if (context.resources && !programName.empty() && !vertexShaderPath.empty() && !fragmentShaderPath.empty()) {
            program = context.resources->loadProgram(
                programName,
                resolveAssetPath(context.assetsRoot, vertexShaderPath).string(),
                resolveAssetPath(context.assetsRoot, fragmentShaderPath).string()
            );
        }

        engine::resources::RMaterialHandle material{};
        material.program = program;
        material.baseColor = deserializeColor(value.value("baseColor", Json::array()), material.baseColor);
        material.renderState = deserializeRenderState(value.value("renderState", Json::object()));
        if (context.resources) {
            for (const auto& texturePath : texturePaths) {
                if (texturePath.empty()) {
                    continue;
                }

                auto texture = context.resources->loadTexture(
                    resolveAssetPath(context.assetsRoot, texturePath).string()
                );
                if (texture.isValid()) {
                    material.textures.push_back(texture);
                }
            }
        }

        return {
            .material = material,
            .programName = programName,
            .vertexShaderPath = vertexShaderPath,
            .fragmentShaderPath = fragmentShaderPath,
            .texturePaths = std::move(texturePaths)
        };
    }

    auto serializeColliderType(engine::components::ColliderType type) -> std::string_view {
        switch (type) {
            case engine::components::ColliderType::Box:
                return "Box";
            case engine::components::ColliderType::Sphere:
                return "Sphere";
            case engine::components::ColliderType::Capsule:
                return "Capsule";
        }

        return "Box";
    }

    auto deserializeColliderType(const Json& value, engine::components::ColliderType fallback) -> engine::components::ColliderType {
        if (value.is_number_integer()) {
            switch (static_cast<engine::components::ColliderType>(value.get<int>())) {
                case engine::components::ColliderType::Box:
                    return engine::components::ColliderType::Box;
                case engine::components::ColliderType::Sphere:
                    return engine::components::ColliderType::Sphere;
                case engine::components::ColliderType::Capsule:
                    return engine::components::ColliderType::Capsule;
            }
        }

        if (!value.is_string()) {
            return fallback;
        }

        const auto type = value.get<std::string>();
        if (type == "Box") {
            return engine::components::ColliderType::Box;
        }
        if (type == "Sphere") {
            return engine::components::ColliderType::Sphere;
        }
        if (type == "Capsule") {
            return engine::components::ColliderType::Capsule;
        }

        return fallback;
    }

    auto serializeRigidbody(const engine::components::IRigidbodyComponent& component) -> Json {
        return Json{
            {"dynamic", component.dynamic},
            {"mass", component.mass},
            {"useGravity", component.useGravity}
        };
    }

    auto deserializeRigidbody(const Json& value) -> engine::components::IRigidbodyComponent {
        engine::components::IRigidbodyComponent component{};
        component.dynamic = value.value("dynamic", component.dynamic);
        component.mass = value.value("mass", component.mass);
        component.useGravity = value.value("useGravity", component.useGravity);
        return component;
    }

    auto serializeCollider(const engine::components::IColliderComponent& component) -> Json {
        return Json{
            {"type", std::string{serializeColliderType(component.type)}},
            {"size", serializeVec3(component.size)},
            {"radius", component.radius},
            {"height", component.height},
            {"trigger", component.trigger}
        };
    }

    auto deserializeCollider(const Json& value) -> engine::components::IColliderComponent {
        engine::components::IColliderComponent component{};
        component.type = deserializeColliderType(value.value("type", Json{}), component.type);
        component.size = deserializeVec3(value.value("size", Json::array()), component.size);
        component.radius = value.value("radius", component.radius);
        component.height = value.value("height", component.height);
        component.trigger = value.value("trigger", component.trigger);
        return component;
    }

    auto makeFallbackEntityId(entt::entity entity) -> std::string {
        return "entity-" + std::to_string(entt::to_integral(entity));
    }
}

auto engine::systems::SYSceneSerializerSystem::createSceneSerializerSystem(const std::filesystem::path &scenesPath) -> SYSceneSerializerPtr {
    return SYSceneSerializerPtr(new SYSceneSerializerSystem(scenesPath), SYSceneSerializerDeleter{});
}

auto engine::systems::SYSceneSerializerSystem::serializeScene(const scene::SCScene &scene) const -> std::expected<void, SYSceneSerializerError> {
    try {
        std::filesystem::create_directories(m_scenesPath);
    } catch (const std::filesystem::filesystem_error& error) {
        return std::unexpected(makeError(FileSystemErrorCode, error.what()));
    }

    const auto filename = sceneFilename(m_scenesPath, scene.getName());
    const auto& registry = scene.getRegistry();
    const auto* entityStorage = registry.storage<entt::entity>();

    Json sceneBody;
    sceneBody["version"] = 1;
    sceneBody["name"] = std::string{scene.getName()};
    sceneBody["activeCamera"] = nullptr;
    sceneBody["entities"] = Json::array();

    const auto activeCamera = scene.getActiveCamera();
    if (activeCamera.isValid()) {
        if (const auto* identity = registry.try_get<components::IIdentityComponent>(activeCamera.handle)) {
            sceneBody["activeCamera"] = identity->id;
        }
    }

    if (entityStorage) {
        for (const auto [entityHandle] : entityStorage->each()) {
            Json entityBody;
            const auto* identity = registry.try_get<components::IIdentityComponent>(entityHandle);

            const auto fallbackId = makeFallbackEntityId(entityHandle);
            entityBody["id"] = identity ? identity->id : fallbackId;
            entityBody["name"] = identity ? identity->name : fallbackId;
            entityBody["components"] = Json::object();

            if (const auto* transform = registry.try_get<components::ITransformComponent>(entityHandle)) {
                entityBody["components"]["Transform"] = serializeTransform(*transform);
            }
            if (const auto* camera = registry.try_get<components::ICameraComponent>(entityHandle)) {
                entityBody["components"]["Camera"] = serializeCamera(*camera);
            }
            if (const auto* mesh = registry.try_get<components::IMeshComponent>(entityHandle)) {
                entityBody["components"]["Mesh"] = serializeMesh(*mesh);
            }
            if (const auto* material = registry.try_get<components::IMaterialComponent>(entityHandle)) {
                entityBody["components"]["Material"] = serializeMaterial(*material);
            }
            if (const auto* rigidbody = registry.try_get<components::IRigidbodyComponent>(entityHandle)) {
                entityBody["components"]["Rigidbody"] = serializeRigidbody(*rigidbody);
            }
            if (const auto* collider = registry.try_get<components::IColliderComponent>(entityHandle)) {
                entityBody["components"]["Collider"] = serializeCollider(*collider);
            }

            sceneBody["entities"].push_back(std::move(entityBody));
        }
    }

    std::ofstream file{filename};
    if (!file.is_open()) {
        return std::unexpected(makeError(FileSystemErrorCode, "Failed to open scene file for writing: " + filename.string()));
    }

    file << sceneBody.dump(4);
    if (!file.good()) {
        return std::unexpected(makeError(FileSystemErrorCode, "Failed to write scene file: " + filename.string()));
    }

    return {};
}

auto engine::systems::SYSceneSerializerSystem::deserializeScene(
    std::string_view sceneName,
    SYSceneDeserializeContext context
) const -> std::expected<scene::SCScene::SScenePtr, SYSceneSerializerError> {
    const auto filename = sceneFilename(m_scenesPath, sceneName);
    std::ifstream file{filename};
    if (!file.is_open()) {
        return std::unexpected(makeError(FileSystemErrorCode, "Failed to open scene file for reading: " + filename.string()));
    }

    Json sceneBody;
    try {
        sceneBody = Json::parse(file);
    } catch (const Json::exception& error) {
        return std::unexpected(makeError(ParseErrorCode, error.what()));
    }

    if (!sceneBody.is_object()) {
        return std::unexpected(makeError(InvalidFormatErrorCode, "Scene root must be a JSON object"));
    }

    const auto loadedSceneName = sceneBody.value("name", std::string{sceneName});
    auto scene = scene::SCScene::createScene(loadedSceneName);
    if (!scene) {
        return std::unexpected(makeError(InvalidFormatErrorCode, "Failed to create scene"));
    }

    std::string activeCameraId;
    if (sceneBody.contains("activeCamera") && sceneBody.at("activeCamera").is_string()) {
        activeCameraId = sceneBody.at("activeCamera").get<std::string>();
    }
    const auto entities = sceneBody.value("entities", Json::array());
    if (!entities.is_array()) {
        return std::unexpected(makeError(InvalidFormatErrorCode, "Scene entities must be an array"));
    }

    std::size_t fallbackIndex = 0;
    for (const auto& entityBody : entities) {
        if (!entityBody.is_object()) {
            continue;
        }

        const auto fallbackId = "entity-" + std::to_string(fallbackIndex++);
        const auto id = entityBody.value("id", fallbackId);
        const auto name = entityBody.value("name", id);
        const auto entity = scene->createEntity();

        scene->addComponent<components::IIdentityComponent>(entity, components::IIdentityComponent{
            .id = id,
            .name = name
        });

        const auto componentsBody = entityBody.value("components", Json::object());
        if (!componentsBody.is_object()) {
            continue;
        }

        if (componentsBody.contains("Transform")) {
            scene->addComponent<components::ITransformComponent>(
                entity,
                deserializeTransform(componentsBody.at("Transform"))
            );
        }
        if (componentsBody.contains("Camera")) {
            scene->addComponent<components::ICameraComponent>(
                entity,
                deserializeCamera(componentsBody.at("Camera"))
            );
            if (id == activeCameraId || (activeCameraId.empty() && !scene->getActiveCamera().isValid())) {
                scene->setActiveCamera(entity);
            }
        }
        if (componentsBody.contains("Mesh")) {
            scene->addComponent<components::IMeshComponent>(
                entity,
                deserializeMesh(componentsBody.at("Mesh"), context)
            );
        }
        if (componentsBody.contains("Material")) {
            scene->addComponent<components::IMaterialComponent>(
                entity,
                deserializeMaterial(componentsBody.at("Material"), context)
            );
        }
        if (componentsBody.contains("Rigidbody")) {
            scene->addComponent<components::IRigidbodyComponent>(
                entity,
                deserializeRigidbody(componentsBody.at("Rigidbody"))
            );
        }
        if (componentsBody.contains("Collider")) {
            scene->addComponent<components::IColliderComponent>(
                entity,
                deserializeCollider(componentsBody.at("Collider"))
            );
        }
    }

    return std::move(scene);
}

engine::systems::SYSceneSerializerSystem::SYSceneSerializerSystem(std::filesystem::path scenesPath) : m_scenesPath(std::move(scenesPath)) {
}
