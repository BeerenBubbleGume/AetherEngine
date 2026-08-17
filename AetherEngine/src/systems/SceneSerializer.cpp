//
// Created by drhaz on 30.06.2026.
//

#include "systems/SceneSerializer.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <new>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>

#include <entt/entity/entity.hpp>
#include <nlohmann/json.hpp>

#include "components/CameraComponent.hpp"
#include "components/IdentityComponent.hpp"
#include "components/MaterialComponent.hpp"
#include "components/MeshComponent.hpp"
#include "components/PhysicsComponents.hpp"
#include "components/TransformComponent.hpp"
#include "security/PathSecurity.hpp"

namespace {
    using Json = nlohmann::json;

    constexpr int FileSystemErrorCode = 1;
    constexpr int ParseErrorCode = 2;
    constexpr int InvalidFormatErrorCode = 3;
    constexpr std::uint64_t MiB = 1024ULL * 1024ULL;
    constexpr std::uint64_t MaxSceneBytes = 16ULL * MiB;
    constexpr std::size_t MaxSceneEntities = 10000;
    constexpr std::size_t MaxJsonNestingDepth = 64;
    constexpr float MaxCoordinateMagnitude = 1'000'000.0f;
    constexpr float MaxScaleMagnitude = 100'000.0f;

    auto makeError(int code, std::string message) -> AetherEngine::systems::SceneSerializerError {
        return {code, std::move(message)};
    }

    auto sceneFilename(std::string_view sceneName) -> std::optional<std::string> {
        if (!AetherEngine::security::isSafeSceneName(sceneName)) {
            return std::nullopt;
        }
        return std::string{sceneName} + ".scene.json";
    }

    auto readSceneText(
        const std::filesystem::path& scenesRoot,
        std::string_view filename
    )
        -> std::expected<std::string, AetherEngine::systems::SceneSerializerError> {
        const auto file = AetherEngine::security::readRegularFileWithin(
            scenesRoot,
            filename,
            MaxSceneBytes
        );
        if (!file) {
            return std::unexpected(makeError(
                FileSystemErrorCode,
                "Failed to securely open scene file: " + file.error().message
            ));
        }
        try {
            return std::string{
                reinterpret_cast<const char*>(file->bytes.data()),
                file->bytes.size()
            };
        } catch (const std::bad_alloc&) {
            return std::unexpected(makeError(FileSystemErrorCode, "Not enough memory to read scene file"));
        }
    }

    auto isJsonNestingWithinLimit(std::string_view text) noexcept -> bool {
        std::size_t depth = 0;
        bool inString = false;
        bool escaped = false;

        for (const char character : text) {
            if (inString) {
                if (escaped) {
                    escaped = false;
                } else if (character == '\\') {
                    escaped = true;
                } else if (character == '"') {
                    inString = false;
                }
                continue;
            }

            if (character == '"') {
                inString = true;
            } else if (character == '{' || character == '[') {
                if (depth >= MaxJsonNestingDepth) {
                    return false;
                }
                ++depth;
            } else if ((character == '}' || character == ']') && depth > 0) {
                --depth;
            }
        }

        return true;
    }

    auto isFiniteBounded(float value, float magnitude) -> bool {
        return std::isfinite(value) && std::abs(value) <= magnitude;
    }

    auto positiveFiniteOr(float value, float fallback, float maximum) -> float {
        return std::isfinite(value) && value > 0.0f && value <= maximum ? value : fallback;
    }

    auto serializeVec3(const AetherEngine::math::Vec3& value) -> Json {
        return Json::array({value.x, value.y, value.z});
    }

    auto deserializeVec3(const Json& value, AetherEngine::math::Vec3 fallback) -> AetherEngine::math::Vec3 {
        if (!value.is_array() || value.size() != 3) {
            return fallback;
        }
        const AetherEngine::math::Vec3 result{
            value.at(0).get<float>(),
            value.at(1).get<float>(),
            value.at(2).get<float>()
        };
        return isFiniteBounded(result.x, MaxCoordinateMagnitude) &&
               isFiniteBounded(result.y, MaxCoordinateMagnitude) &&
               isFiniteBounded(result.z, MaxCoordinateMagnitude)
            ? result
            : fallback;
    }

    auto serializeQuat(const AetherEngine::math::Quat& value) -> Json {
        return Json::array({value.w, value.x, value.y, value.z});
    }

    auto deserializeQuat(const Json& value, AetherEngine::math::Quat fallback) -> AetherEngine::math::Quat {
        if (!value.is_array() || value.size() != 4) {
            return fallback;
        }
        const AetherEngine::math::Quat result{
            value.at(0).get<float>(),
            value.at(1).get<float>(),
            value.at(2).get<float>(),
            value.at(3).get<float>()
        };
        if (!isFiniteBounded(result.w, 1.0f) ||
            !isFiniteBounded(result.x, 1.0f) ||
            !isFiniteBounded(result.y, 1.0f) ||
            !isFiniteBounded(result.z, 1.0f) ||
            result.length() < 0.000001f) {
            return fallback;
        }
        return result.normalized();
    }

    auto serializeColor(const AetherEngine::math::Color& value) -> Json {
        return Json::array({value.r, value.g, value.b, value.a});
    }

    auto deserializeColor(const Json& value, AetherEngine::math::Color fallback) -> AetherEngine::math::Color {
        if (!value.is_array() || value.size() != 4) {
            return fallback;
        }
        const AetherEngine::math::Color result{
            value.at(0).get<float>(),
            value.at(1).get<float>(),
            value.at(2).get<float>(),
            value.at(3).get<float>()
        };
        return isFiniteBounded(result.r, 1.0f) &&
               isFiniteBounded(result.g, 1.0f) &&
               isFiniteBounded(result.b, 1.0f) &&
               isFiniteBounded(result.a, 1.0f)
            ? result
            : fallback;
    }

    auto serializeTransform(const AetherEngine::components::TransformComponent& component) -> Json {
        return Json{
            {"position", serializeVec3(component.transform.position)},
            {"rotation", serializeQuat(component.transform.rotation)},
            {"scale", serializeVec3(component.transform.scale)}
        };
    }

    auto deserializeTransform(const Json& value) -> AetherEngine::components::TransformComponent {
        AetherEngine::components::TransformComponent component{};
        component.transform.position = deserializeVec3(value.value("position", Json::array()), AetherEngine::math::Vec3::zero());
        component.transform.rotation = deserializeQuat(value.value("rotation", Json::array()), AetherEngine::math::Quat::identity());
        component.transform.scale = deserializeVec3(value.value("scale", Json::array()), AetherEngine::math::Vec3::one());
        if (!isFiniteBounded(component.transform.scale.x, MaxScaleMagnitude) ||
            !isFiniteBounded(component.transform.scale.y, MaxScaleMagnitude) ||
            !isFiniteBounded(component.transform.scale.z, MaxScaleMagnitude)) {
            component.transform.scale = AetherEngine::math::Vec3::one();
        }
        return component;
    }

    auto serializeCamera(const AetherEngine::components::CameraComponent& component) -> Json {
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

    auto deserializeCamera(const Json& value) -> AetherEngine::components::CameraComponent {
        AetherEngine::components::CameraComponent component{};
        component.enabled = value.value("enabled", component.enabled);
        const auto projection = value.value("projection", static_cast<int>(component.projection));
        if (projection == static_cast<int>(AetherEngine::components::ProjectionType::Perspective) ||
            projection == static_cast<int>(AetherEngine::components::ProjectionType::Orthographic)) {
            component.projection = static_cast<AetherEngine::components::ProjectionType>(projection);
        }
        component.fovYDegrees = positiveFiniteOr(
            value.value("fovYDegrees", component.fovYDegrees),
            component.fovYDegrees,
            179.0f
        );
        component.nearPlane = positiveFiniteOr(
            value.value("nearPlane", component.nearPlane),
            component.nearPlane,
            MaxCoordinateMagnitude
        );
        component.farPlane = positiveFiniteOr(
            value.value("farPlane", component.farPlane),
            component.farPlane,
            MaxCoordinateMagnitude
        );
        if (component.farPlane <= component.nearPlane) {
            component.nearPlane = 0.1f;
            component.farPlane = 100.0f;
        }
        component.orthographicHeight = positiveFiniteOr(
            value.value("orthographicHeight", component.orthographicHeight),
            component.orthographicHeight,
            MaxCoordinateMagnitude
        );
        component.clearFlags = value.value("clearFlags", component.clearFlags);
        component.clearFlags &= static_cast<uint8_t>(
            AetherEngine::components::ClearColor |
            AetherEngine::components::ClearDepth |
            AetherEngine::components::ClearStencil
        );
        component.clearColor = value.value("clearColor", component.clearColor);
        component.priority = value.value("priority", component.priority);
        component.layerMask = value.value("layerMask", component.layerMask);
        return component;
    }

    auto serializeMesh(const AetherEngine::components::MeshComponent& component) -> Json {
        return Json{
            {"asset", AetherEngine::assets::toString(component.asset.id)}
        };
    }

    auto deserializeMesh(
        const Json& value,
        const AetherEngine::systems::SceneDeserializeContext& context
    ) -> AetherEngine::components::MeshComponent {
        const auto id = AetherEngine::assets::parseAssetId(value.value("asset", std::string{}));
        if (!id) {
            throw std::runtime_error{"Mesh component contains an invalid asset id"};
        }

        const AetherEngine::assets::AssetRef<AetherEngine::assets::RuntimeMeshAsset> asset{*id};
        AetherEngine::assets::AssetHandle<AetherEngine::assets::RuntimeMeshAsset> runtime{};
        if (context.assets) {
            const auto loaded = context.assets->load(asset);
            if (!loaded) {
                throw std::runtime_error{"Failed to load mesh asset: " + loaded.error().message};
            }
            runtime = *loaded;
        }
        return {
            .asset = asset,
            .runtime = runtime
        };
    }

    auto serializeMaterial(const AetherEngine::components::MaterialComponent& component) -> Json {
        return Json{
            {"asset", AetherEngine::assets::toString(component.asset.id)},
            {"baseColor", serializeColor(component.baseColor)}
        };
    }

    auto deserializeMaterial(
        const Json& value,
        const AetherEngine::systems::SceneDeserializeContext& context
    ) -> AetherEngine::components::MaterialComponent {
        const auto id = AetherEngine::assets::parseAssetId(value.value("asset", std::string{}));
        if (!id) {
            throw std::runtime_error{"Material component contains an invalid asset id"};
        }

        const AetherEngine::assets::AssetRef<AetherEngine::assets::RuntimeMaterialAsset> asset{*id};
        AetherEngine::assets::AssetHandle<AetherEngine::assets::RuntimeMaterialAsset> runtime{};
        if (context.assets) {
            const auto loaded = context.assets->load(asset);
            if (!loaded) {
                throw std::runtime_error{"Failed to load material asset: " + loaded.error().message};
            }
            runtime = *loaded;
        }

        return {
            .asset = asset,
            .runtime = runtime,
            .baseColor = deserializeColor(
                value.value("baseColor", Json::array()),
                AetherEngine::math::Color{1.0f, 1.0f, 1.0f, 1.0f}
            )
        };
    }

    auto serializeColliderType(AetherEngine::components::ColliderType type) -> std::string_view {
        switch (type) {
            case AetherEngine::components::ColliderType::Box:
                return "Box";
            case AetherEngine::components::ColliderType::Sphere:
                return "Sphere";
            case AetherEngine::components::ColliderType::Capsule:
                return "Capsule";
        }

        return "Box";
    }

    auto deserializeColliderType(const Json& value, AetherEngine::components::ColliderType fallback) -> AetherEngine::components::ColliderType {
        if (value.is_number_integer()) {
            switch (static_cast<AetherEngine::components::ColliderType>(value.get<int>())) {
                case AetherEngine::components::ColliderType::Box:
                    return AetherEngine::components::ColliderType::Box;
                case AetherEngine::components::ColliderType::Sphere:
                    return AetherEngine::components::ColliderType::Sphere;
                case AetherEngine::components::ColliderType::Capsule:
                    return AetherEngine::components::ColliderType::Capsule;
            }
        }

        if (!value.is_string()) {
            return fallback;
        }

        const auto type = value.get<std::string>();
        if (type == "Box") {
            return AetherEngine::components::ColliderType::Box;
        }
        if (type == "Sphere") {
            return AetherEngine::components::ColliderType::Sphere;
        }
        if (type == "Capsule") {
            return AetherEngine::components::ColliderType::Capsule;
        }

        return fallback;
    }

    auto serializeRigidbody(const AetherEngine::components::RigidbodyComponent& component) -> Json {
        return Json{
            {"dynamic", component.dynamic},
            {"mass", component.mass},
            {"useGravity", component.useGravity}
        };
    }

    auto deserializeRigidbody(const Json& value) -> AetherEngine::components::RigidbodyComponent {
        AetherEngine::components::RigidbodyComponent component{};
        component.dynamic = value.value("dynamic", component.dynamic);
        component.mass = positiveFiniteOr(
            value.value("mass", component.mass),
            component.mass,
            1'000'000'000.0f
        );
        component.useGravity = value.value("useGravity", component.useGravity);
        return component;
    }

    auto serializeCollider(const AetherEngine::components::ColliderComponent& component) -> Json {
        return Json{
            {"type", std::string{serializeColliderType(component.type)}},
            {"size", serializeVec3(component.size)},
            {"radius", component.radius},
            {"height", component.height},
            {"trigger", component.trigger}
        };
    }

    auto deserializeCollider(const Json& value) -> AetherEngine::components::ColliderComponent {
        AetherEngine::components::ColliderComponent component{};
        component.type = deserializeColliderType(value.value("type", Json{}), component.type);
        component.size = deserializeVec3(value.value("size", Json::array()), component.size);
        if (component.size.x <= 0.0f || component.size.y <= 0.0f || component.size.z <= 0.0f ||
            component.size.x > MaxScaleMagnitude ||
            component.size.y > MaxScaleMagnitude ||
            component.size.z > MaxScaleMagnitude) {
            component.size = AetherEngine::math::Vec3::one();
        }
        component.radius = positiveFiniteOr(
            value.value("radius", component.radius),
            component.radius,
            MaxScaleMagnitude
        );
        component.height = positiveFiniteOr(
            value.value("height", component.height),
            component.height,
            MaxScaleMagnitude
        );
        component.trigger = value.value("trigger", component.trigger);
        return component;
    }

    auto makeFallbackEntityId(entt::entity entity) -> std::string {
        return "entity-" + std::to_string(entt::to_integral(entity));
    }
}

auto AetherEngine::systems::SceneSerializer::createSceneSerializer(const std::filesystem::path &scenesPath) -> SceneSerializerPtr {
    const auto canonicalParent = security::canonicalDirectory(scenesPath.parent_path());
    const auto directoryName = scenesPath.filename().string();
    if (!canonicalParent || !security::isSafeSceneName(directoryName)) {
        return {};
    }

    const auto canonicalScenes = security::resolveDirectoryWithin(
        *canonicalParent,
        directoryName
    );
    if (!canonicalScenes) {
        return {};
    }
    return SceneSerializerPtr(new SceneSerializer(*canonicalScenes), SceneSerializerDeleter{});
}

auto AetherEngine::systems::SceneSerializer::serializeScene(const scene::Scene &scene) const -> std::expected<void, SceneSerializerError> {
    const auto currentScenesPath = security::resolveDirectoryWithin(
        m_scenesPath.parent_path(),
        m_scenesPath.filename().string()
    );
    if (!currentScenesPath || *currentScenesPath != m_scenesPath) {
        return std::unexpected(makeError(FileSystemErrorCode, "Scene directory is no longer the trusted canonical directory"));
    }

    const auto filename = sceneFilename(scene.getName());
    if (!filename) {
        return std::unexpected(makeError(InvalidFormatErrorCode, "Scene name is not a safe file name"));
    }
    const auto& registry = scene.getRegistry();
    const auto* entityStorage = registry.storage<entt::entity>();
    if (entityStorage && entityStorage->size() > MaxSceneEntities) {
        return std::unexpected(makeError(InvalidFormatErrorCode, "Scene exceeds the 10000 entity limit"));
    }

    Json sceneBody;
    sceneBody["version"] = 2;
    sceneBody["name"] = std::string{scene.getName()};
    sceneBody["activeCamera"] = nullptr;
    sceneBody["entities"] = Json::array();

    const auto activeCamera = scene.getActiveCamera();
    if (activeCamera.isValid()) {
        if (const auto* identity = registry.try_get<components::IdentityComponent>(activeCamera.handle)) {
            sceneBody["activeCamera"] = identity->id;
        }
    }

    if (entityStorage) {
        for (const auto [entityHandle] : entityStorage->each()) {
            Json entityBody;
            const auto* identity = registry.try_get<components::IdentityComponent>(entityHandle);

            const auto fallbackId = makeFallbackEntityId(entityHandle);
            entityBody["id"] = identity ? identity->id : fallbackId;
            entityBody["name"] = identity ? identity->name : fallbackId;
            entityBody["components"] = Json::object();

            if (const auto* transform = registry.try_get<components::TransformComponent>(entityHandle)) {
                entityBody["components"]["Transform"] = serializeTransform(*transform);
            }
            if (const auto* camera = registry.try_get<components::CameraComponent>(entityHandle)) {
                entityBody["components"]["Camera"] = serializeCamera(*camera);
            }
            if (const auto* mesh = registry.try_get<components::MeshComponent>(entityHandle)) {
                entityBody["components"]["Mesh"] = serializeMesh(*mesh);
            }
            if (const auto* material = registry.try_get<components::MaterialComponent>(entityHandle)) {
                entityBody["components"]["Material"] = serializeMaterial(*material);
            }
            if (const auto* rigidbody = registry.try_get<components::RigidbodyComponent>(entityHandle)) {
                entityBody["components"]["Rigidbody"] = serializeRigidbody(*rigidbody);
            }
            if (const auto* collider = registry.try_get<components::ColliderComponent>(entityHandle)) {
                entityBody["components"]["Collider"] = serializeCollider(*collider);
            }

            sceneBody["entities"].push_back(std::move(entityBody));
        }
    }

    std::string serialized;
    try {
        serialized = sceneBody.dump(4);
    } catch (const std::bad_alloc&) {
        return std::unexpected(makeError(FileSystemErrorCode, "Not enough memory to serialize scene"));
    }
    if (serialized.size() > MaxSceneBytes) {
        return std::unexpected(makeError(InvalidFormatErrorCode, "Serialized scene exceeds the 16 MiB limit"));
    }

    const auto writeResult = security::writeRegularFileWithin(
        m_scenesPath,
        *filename,
        std::span<const std::uint8_t>{
            reinterpret_cast<const std::uint8_t*>(serialized.data()),
            serialized.size()
        },
        MaxSceneBytes
    );
    if (!writeResult) {
        return std::unexpected(makeError(
            FileSystemErrorCode,
            "Failed to securely write scene file: " + writeResult.error().message
        ));
    }

    return {};
}

auto AetherEngine::systems::SceneSerializer::deserializeScene(
    std::string_view sceneName,
    SceneDeserializeContext context
) const -> std::expected<scene::Scene::ScenePtr, SceneSerializerError> {
    const auto currentScenesPath = security::resolveDirectoryWithin(
        m_scenesPath.parent_path(),
        m_scenesPath.filename().string()
    );
    if (!currentScenesPath || *currentScenesPath != m_scenesPath) {
        return std::unexpected(makeError(FileSystemErrorCode, "Scene directory is no longer the trusted canonical directory"));
    }
    const auto filename = sceneFilename(sceneName);
    if (!filename) {
        return std::unexpected(makeError(InvalidFormatErrorCode, "Scene name is not a safe file name"));
    }
    const auto sceneText = readSceneText(m_scenesPath, *filename);
    if (!sceneText) {
        return std::unexpected(sceneText.error());
    }
    if (!isJsonNestingWithinLimit(*sceneText)) {
        return std::unexpected(makeError(
            InvalidFormatErrorCode,
            "Scene JSON exceeds the maximum nesting depth of 64"
        ));
    }

    Json sceneBody;
    try {
        sceneBody = Json::parse(*sceneText);
    } catch (const std::bad_alloc&) {
        return std::unexpected(makeError(ParseErrorCode, "Not enough memory to parse scene JSON"));
    } catch (const Json::exception& error) {
        return std::unexpected(makeError(ParseErrorCode, error.what()));
    }

    try {
        if (!sceneBody.is_object()) {
            return std::unexpected(makeError(InvalidFormatErrorCode, "Scene root must be a JSON object"));
        }
        if (!sceneBody.contains("version") || !sceneBody.at("version").is_number_integer() ||
            sceneBody.at("version").get<int>() != 2) {
            return std::unexpected(makeError(InvalidFormatErrorCode, "Unsupported or missing scene version"));
        }

        const auto loadedSceneName = sceneBody.value("name", std::string{sceneName});
        if (!security::isSafeSceneName(loadedSceneName)) {
            return std::unexpected(makeError(InvalidFormatErrorCode, "Serialized scene name is not safe"));
        }
        auto scene = scene::Scene::createScene(loadedSceneName);
        if (!scene) {
            return std::unexpected(makeError(InvalidFormatErrorCode, "Failed to create scene"));
        }

        std::string activeCameraId;
        if (sceneBody.contains("activeCamera") && sceneBody.at("activeCamera").is_string()) {
            activeCameraId = sceneBody.at("activeCamera").get<std::string>();
            if (activeCameraId.size() > 256) {
                return std::unexpected(makeError(InvalidFormatErrorCode, "Active camera id is too long"));
            }
        }
        const auto entitiesIt = sceneBody.find("entities");
        if (entitiesIt != sceneBody.end() && !entitiesIt->is_array()) {
            return std::unexpected(makeError(InvalidFormatErrorCode, "Scene entities must be an array"));
        }
        if (entitiesIt != sceneBody.end() && entitiesIt->size() > MaxSceneEntities) {
            return std::unexpected(makeError(InvalidFormatErrorCode, "Scene exceeds the 10000 entity limit"));
        }

        std::unordered_set<std::string> entityIds;
        if (entitiesIt != sceneBody.end()) {
            entityIds.reserve(entitiesIt->size());
        }
        std::size_t fallbackIndex = 0;
        if (entitiesIt != sceneBody.end()) {
            for (const auto& entityBody : *entitiesIt) {
                if (!entityBody.is_object()) {
                    continue;
                }

                const auto fallbackId = "entity-" + std::to_string(fallbackIndex++);
                const auto id = entityBody.value("id", fallbackId);
                const auto name = entityBody.value("name", id);
                if (id.size() > 256 || name.size() > 1024) {
                    return std::unexpected(makeError(InvalidFormatErrorCode, "Entity id or name is too long"));
                }
                if (!entityIds.emplace(id).second) {
                    return std::unexpected(makeError(InvalidFormatErrorCode, "Scene contains duplicate entity ids"));
                }
                const auto entity = scene->createEntity();

                scene->addComponent<components::IdentityComponent>(entity, components::IdentityComponent{
                    .id = id,
                    .name = name
                });

                const auto componentsIt = entityBody.find("components");
                if (componentsIt == entityBody.end()) {
                    continue;
                }
                if (!componentsIt->is_object()) {
                    continue;
                }
                const auto& componentsBody = *componentsIt;

                if (componentsBody.contains("Transform")) {
                    scene->addComponent<components::TransformComponent>(
                        entity,
                        deserializeTransform(componentsBody.at("Transform"))
                    );
                }
                if (componentsBody.contains("Camera")) {
                    scene->addComponent<components::CameraComponent>(
                        entity,
                        deserializeCamera(componentsBody.at("Camera"))
                    );
                    if (id == activeCameraId || (activeCameraId.empty() && !scene->getActiveCamera().isValid())) {
                        scene->setActiveCamera(entity);
                    }
                }
                if (componentsBody.contains("Mesh")) {
                    scene->addComponent<components::MeshComponent>(
                        entity,
                        deserializeMesh(componentsBody.at("Mesh"), context)
                    );
                }
                if (componentsBody.contains("Material")) {
                    scene->addComponent<components::MaterialComponent>(
                        entity,
                        deserializeMaterial(componentsBody.at("Material"), context)
                    );
                }
                if (componentsBody.contains("Rigidbody")) {
                    scene->addComponent<components::RigidbodyComponent>(
                        entity,
                        deserializeRigidbody(componentsBody.at("Rigidbody"))
                    );
                }
                if (componentsBody.contains("Collider")) {
                    scene->addComponent<components::ColliderComponent>(
                        entity,
                        deserializeCollider(componentsBody.at("Collider"))
                    );
                }
            }
        }

        return scene;
    } catch (const std::bad_alloc&) {
        return std::unexpected(makeError(InvalidFormatErrorCode, "Not enough memory to deserialize scene"));
    } catch (const Json::exception& error) {
        return std::unexpected(makeError(InvalidFormatErrorCode, error.what()));
    } catch (const std::exception& error) {
        return std::unexpected(makeError(InvalidFormatErrorCode, error.what()));
    }
}

AetherEngine::systems::SceneSerializer::SceneSerializer(std::filesystem::path scenesPath) : m_scenesPath(std::move(scenesPath)) {
}
