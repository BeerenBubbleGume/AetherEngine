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
    constexpr std::size_t MaxTexturesPerMaterial = 16;
    constexpr std::size_t MaxAssetPathLength = 4096;
    constexpr std::size_t MaxJsonNestingDepth = 64;
    constexpr float MaxCoordinateMagnitude = 1'000'000.0f;
    constexpr float MaxScaleMagnitude = 100'000.0f;

    auto makeError(int code, std::string message) -> engine::systems::SceneSerializerError {
        return {code, std::move(message)};
    }

    auto sceneFilename(std::string_view sceneName) -> std::optional<std::string> {
        if (!engine::security::isSafeSceneName(sceneName)) {
            return std::nullopt;
        }
        return std::string{sceneName} + ".scene.json";
    }

    auto resolveAssetPath(const std::filesystem::path& assetsRoot, std::string_view assetPath)
        -> std::optional<std::filesystem::path> {
        if (assetPath.empty() || assetPath.size() > MaxAssetPathLength ||
            std::ranges::any_of(assetPath, [](const unsigned char character) {
                return character < 0x20 || character == 0x7f;
            })) {
            return std::nullopt;
        }
        std::filesystem::path path{assetPath};
        if (path.is_absolute() || path.has_root_path() || assetPath.contains(':')) {
            return std::nullopt;
        }
        path = path.lexically_normal();
        if (path.empty() || path == "." ||
            (path.begin() != path.end() && *path.begin() == "..")) {
            return std::nullopt;
        }
        static_cast<void>(assetsRoot);
        return path;
    }

    auto readSceneText(
        const std::filesystem::path& scenesRoot,
        std::string_view filename
    )
        -> std::expected<std::string, engine::systems::SceneSerializerError> {
        const auto file = engine::security::readRegularFileWithin(
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

    auto serializeVec3(const engine::math::Vec3& value) -> Json {
        return Json::array({value.x, value.y, value.z});
    }

    auto deserializeVec3(const Json& value, engine::math::Vec3 fallback) -> engine::math::Vec3 {
        if (!value.is_array() || value.size() != 3) {
            return fallback;
        }
        const engine::math::Vec3 result{
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

    auto serializeQuat(const engine::math::Quat& value) -> Json {
        return Json::array({value.w, value.x, value.y, value.z});
    }

    auto deserializeQuat(const Json& value, engine::math::Quat fallback) -> engine::math::Quat {
        if (!value.is_array() || value.size() != 4) {
            return fallback;
        }
        const engine::math::Quat result{
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

    auto serializeColor(const engine::math::Color& value) -> Json {
        return Json::array({value.r, value.g, value.b, value.a});
    }

    auto deserializeColor(const Json& value, engine::math::Color fallback) -> engine::math::Color {
        if (!value.is_array() || value.size() != 4) {
            return fallback;
        }
        const engine::math::Color result{
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

    auto serializeTransform(const engine::components::TransformComponent& component) -> Json {
        return Json{
            {"position", serializeVec3(component.transform.position)},
            {"rotation", serializeQuat(component.transform.rotation)},
            {"scale", serializeVec3(component.transform.scale)}
        };
    }

    auto deserializeTransform(const Json& value) -> engine::components::TransformComponent {
        engine::components::TransformComponent component{};
        component.transform.position = deserializeVec3(value.value("position", Json::array()), engine::math::Vec3::zero());
        component.transform.rotation = deserializeQuat(value.value("rotation", Json::array()), engine::math::Quat::identity());
        component.transform.scale = deserializeVec3(value.value("scale", Json::array()), engine::math::Vec3::one());
        if (!isFiniteBounded(component.transform.scale.x, MaxScaleMagnitude) ||
            !isFiniteBounded(component.transform.scale.y, MaxScaleMagnitude) ||
            !isFiniteBounded(component.transform.scale.z, MaxScaleMagnitude)) {
            component.transform.scale = engine::math::Vec3::one();
        }
        return component;
    }

    auto serializeCamera(const engine::components::CameraComponent& component) -> Json {
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

    auto deserializeCamera(const Json& value) -> engine::components::CameraComponent {
        engine::components::CameraComponent component{};
        component.enabled = value.value("enabled", component.enabled);
        const auto projection = value.value("projection", static_cast<int>(component.projection));
        if (projection == static_cast<int>(engine::components::ProjectionType::Perspective) ||
            projection == static_cast<int>(engine::components::ProjectionType::Orthographic)) {
            component.projection = static_cast<engine::components::ProjectionType>(projection);
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
            engine::components::ClearColor |
            engine::components::ClearDepth |
            engine::components::ClearStencil
        );
        component.clearColor = value.value("clearColor", component.clearColor);
        component.priority = value.value("priority", component.priority);
        component.layerMask = value.value("layerMask", component.layerMask);
        return component;
    }

    auto serializeRenderState(const engine::resources::RenderState& value) -> Json {
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
            auto path = value.get<std::string>();
            if (path.size() <= MaxAssetPathLength) {
                texturePaths.push_back(std::move(path));
            }
            return texturePaths;
        }

        if (!value.is_array()) {
            return texturePaths;
        }

        for (const auto& texturePath : value) {
            if (texturePaths.size() >= MaxTexturesPerMaterial) {
                break;
            }
            if (texturePath.is_string()) {
                auto path = texturePath.get<std::string>();
                if (path.size() <= MaxAssetPathLength) {
                    texturePaths.push_back(std::move(path));
                }
            }
        }

        return texturePaths;
    }

    auto deserializeRenderState(const Json& value) -> engine::resources::RenderState {
        engine::resources::RenderState state{};
        state.writeRgb = value.value("writeRgb", state.writeRgb);
        state.writeAlpha = value.value("writeAlpha", state.writeAlpha);
        state.writeDepth = value.value("writeDepth", state.writeDepth);
        state.depthTest = value.value("depthTest", state.depthTest);
        state.cullBackFaces = value.value("cullBackFaces", state.cullBackFaces);
        state.alphaBlend = value.value("alphaBlend", state.alphaBlend);
        state.msaa = value.value("msaa", state.msaa);
        return state;
    }

    auto serializeMesh(const engine::components::MeshComponent& component) -> Json {
        return Json{
            {"assetPath", component.assetPath}
        };
    }

    auto deserializeMesh(
        const Json& value,
        const engine::systems::SceneDeserializeContext& context
    ) -> engine::components::MeshComponent {
        const auto assetPath = value.value("assetPath", std::string{});
        engine::resources::MeshHandle mesh{};
        if (context.resources && !assetPath.empty()) {
            if (const auto resolved = resolveAssetPath(context.assetsRoot, assetPath)) {
                mesh = context.resources->loadMesh(resolved->string());
            }
        }
        return {
            .mesh = mesh,
            .assetPath = assetPath
        };
    }

    auto serializeMaterial(const engine::components::MaterialComponent& component) -> Json {
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
        const engine::systems::SceneDeserializeContext& context
    ) -> engine::components::MaterialComponent {
        const auto programName = value.value("programName", std::string{});
        const auto vertexShaderPath = value.value("vertexShaderPath", std::string{});
        const auto fragmentShaderPath = value.value("fragmentShaderPath", std::string{});
        std::vector<std::string> texturePaths;
        if (value.contains("textures")) {
            texturePaths = deserializeTexturePaths(value.at("textures"));
        } else if (value.contains("texturePaths")) {
            texturePaths = deserializeTexturePaths(value.at("texturePaths"));
        }

        engine::resources::ProgramHandle program{};
        if (context.resources && !programName.empty() && !vertexShaderPath.empty() && !fragmentShaderPath.empty()) {
            const auto vertexPath = resolveAssetPath(context.assetsRoot, vertexShaderPath);
            const auto fragmentPath = resolveAssetPath(context.assetsRoot, fragmentShaderPath);
            if (vertexPath && fragmentPath) {
                program = context.resources->loadProgram(
                    programName,
                    vertexPath->string(),
                    fragmentPath->string()
                );
            }
        }

        engine::resources::Material material{};
        material.program = program;
        material.baseColor = deserializeColor(value.value("baseColor", Json::array()), material.baseColor);
        material.renderState = deserializeRenderState(value.value("renderState", Json::object()));
        if (context.resources) {
            for (const auto& texturePath : texturePaths) {
                if (texturePath.empty()) {
                    continue;
                }

                const auto resolved = resolveAssetPath(context.assetsRoot, texturePath);
                if (!resolved) {
                    continue;
                }
                auto texture = context.resources->loadTexture(resolved->string());
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

    auto serializeRigidbody(const engine::components::RigidbodyComponent& component) -> Json {
        return Json{
            {"dynamic", component.dynamic},
            {"mass", component.mass},
            {"useGravity", component.useGravity}
        };
    }

    auto deserializeRigidbody(const Json& value) -> engine::components::RigidbodyComponent {
        engine::components::RigidbodyComponent component{};
        component.dynamic = value.value("dynamic", component.dynamic);
        component.mass = positiveFiniteOr(
            value.value("mass", component.mass),
            component.mass,
            1'000'000'000.0f
        );
        component.useGravity = value.value("useGravity", component.useGravity);
        return component;
    }

    auto serializeCollider(const engine::components::ColliderComponent& component) -> Json {
        return Json{
            {"type", std::string{serializeColliderType(component.type)}},
            {"size", serializeVec3(component.size)},
            {"radius", component.radius},
            {"height", component.height},
            {"trigger", component.trigger}
        };
    }

    auto deserializeCollider(const Json& value) -> engine::components::ColliderComponent {
        engine::components::ColliderComponent component{};
        component.type = deserializeColliderType(value.value("type", Json{}), component.type);
        component.size = deserializeVec3(value.value("size", Json::array()), component.size);
        if (component.size.x <= 0.0f || component.size.y <= 0.0f || component.size.z <= 0.0f ||
            component.size.x > MaxScaleMagnitude ||
            component.size.y > MaxScaleMagnitude ||
            component.size.z > MaxScaleMagnitude) {
            component.size = engine::math::Vec3::one();
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

auto engine::systems::SceneSerializer::createSceneSerializer(const std::filesystem::path &scenesPath) -> SceneSerializerPtr {
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

auto engine::systems::SceneSerializer::serializeScene(const scene::Scene &scene) const -> std::expected<void, SceneSerializerError> {
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
    sceneBody["version"] = 1;
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

auto engine::systems::SceneSerializer::deserializeScene(
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
            sceneBody.at("version").get<int>() != 1) {
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
    }
}

engine::systems::SceneSerializer::SceneSerializer(std::filesystem::path scenesPath) : m_scenesPath(std::move(scenesPath)) {
}
