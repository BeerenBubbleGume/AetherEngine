#include "SceneWriteTestSupport.hpp"
#include "systems/SceneAssetBindingSystem.hpp"
#include "systems/SceneSerializer.hpp"
#include "components/MeshComponent.hpp"
#include "components/MaterialComponent.hpp"
#include <bgfx/bgfx.h>
#include <nlohmann/json.hpp>

namespace {
    using namespace scene_write_tests;
    namespace ae = AetherEngine;
    namespace assets = ae::assets;
    using Scene = ae::scene::Scene;
    constexpr assets::AssetId Good{1, 1};
    constexpr assets::AssetId Broken{2, 2};
    constexpr assets::AssetId Material{3, 3};
    constexpr assets::AssetId Unknown{99, 99};

    void bindingRecovery() {
        Fixture files;
        const auto root = files.root / "assets";
        fs::create_directories(root / "registry");
        fs::create_directories(root / "meshes");
        fs::create_directories(root / "materials");
        const auto cube = fs::path{AETHER_TEST_ASSETS_ROOT} / "meshes/bin/primitives/cube.bin";
        fs::copy_file(cube, root / "meshes/good.bin");
        put(root / "meshes/broken.bin", "not a mesh");
        put(root / "materials/broken.json", "invalid material");
        nlohmann::json records = nlohmann::json::array();
        const auto addRecord = [&](assets::AssetId id, const char* type, const char* alias, const char* path) {
            records.push_back({{"id", assets::toString(id)}, {"alias", alias}, {"type", type}, {"dependencies", nlohmann::json::array()},
                {"variants", {{{"platform", {{"os", "any"}, {"architecture", "any"}, {"graphics", "any"}}}, {"path", path}}}}});
        };
        addRecord(Good, "mesh", "asset://meshes/good", "meshes/good.bin");
        addRecord(Broken, "mesh", "asset://meshes/broken", "meshes/broken.bin");
        addRecord(Material, "material", "asset://materials/broken", "materials/broken.json");
        put(root / "registry/runtime-assets.json", nlohmann::json{{"version", 1}, {"assets", records}}.dump());
        auto resources = ae::resources::ResourceManager::createResourceManager(root);
        auto created = assets::AssetManager::create(root, *resources,
            {assets::OperatingSystem::Windows, assets::Architecture::X64, assets::GraphicsBackend::Direct3D11});
        require(created.has_value(), "Asset manager setup failed");
        auto manager = std::move(*created);
        auto bindings = ae::systems::SceneAssetBindingSystem::create(*manager);
        auto scene = Scene::createScene("Assets");
        const auto good = scene->createEntity("Good mesh");
        scene->addComponent<ae::components::MeshComponent>(good).asset.id = Good;
        require(bindings->bindScene(*scene).has_value(), "Initial strict binding failed");
        const auto goodHandle = bindings->meshHandle({Good});
        require(goodHandle && manager->get(*goodHandle), "Valid mesh did not bind");

        const auto broken = scene->createEntity("Broken mesh");
        scene->addComponent<ae::components::MeshComponent>(broken).asset.id = Broken;
        const auto unknown = scene->createEntity("Missing material");
        scene->addComponent<ae::components::MaterialComponent>(unknown).asset.id = Unknown;
        const auto badMaterial = scene->createEntity("Broken material");
        scene->addComponent<ae::components::MaterialComponent>(badMaterial).asset.id = Material;
        require(!bindings->synchronize(*scene), "Strict runtime mode accepted missing assets");
        require(manager->get(*goodHandle) != nullptr && bindings->errors().empty(), "Failed strict bind altered committed state");

        bindings->setAllowMissingAssets(true);
        for (int frame = 0; frame < 5; ++frame) {
            require(bindings->synchronize(*scene).has_value(), "Editor synchronization failed on a resource error");
            require(bindings->errors().size() == 3, "Editor did not report all missing/broken assets");
            require(bindings->meshHandle({Good}).has_value() && manager->get(*goodHandle), "An invalid object hid the valid mesh");
            require(!bindings->meshHandle({Broken}) && !bindings->materialHandle({Unknown}) && !bindings->materialHandle({Material}),
                "An unavailable resource received a render binding");
        }
        auto serializer = ae::systems::SceneSerializer::createSceneSerializer(files.scenes);
        require(serializer->serializeScene(*scene, "Unresolved").has_value(), "Unresolved assets prevented saving the document");
        auto reopened = serializer->deserializeScene("Unresolved");
        require(reopened.has_value(), "Scene with missing assets could not be reopened");
        require(bindings->bindScene(**reopened).has_value() && bindings->errors().size() == 3,
            "Authoring bind rejected a newly opened scene with missing resources");

        // Correcting a file is retried explicitly; ready handles remain valid.
        fs::copy_file(cube, root / "meshes/broken.bin", fs::copy_options::overwrite_existing);
        require(bindings->synchronize(**reopened).has_value() && !bindings->meshHandle({Broken}), "Failed-load cache unexpectedly retried itself");
        manager->retryFailedLoads();
        require(bindings->synchronize(**reopened).has_value() && bindings->meshHandle({Broken}).has_value(), "Retry did not recover the repaired mesh");
        require(manager->get(*goodHandle) != nullptr, "Retry invalidated a working resource");
        require(bindings->errors().size() == 2, "Recovery retained the old error");

        const auto unknownId = scene->getComponent<ae::components::IdentityComponent>(unknown).id;
        const auto materialId = scene->getComponent<ae::components::IdentityComponent>(badMaterial).id;
        (*reopened)->destroyEntity(*(*reopened)->findEntityById(unknownId));
        (*reopened)->destroyEntity(*(*reopened)->findEntityById(materialId));
        require(bindings->synchronize(**reopened).has_value() && bindings->errors().empty(), "Removed bad references left stale errors");
        auto empty = Scene::createScene("Empty");
        require(bindings->bindScene(*empty).has_value() && !bindings->meshHandle({Good}), "Scene replacement retained stale bindings");
        require(manager->get(*goodHandle) == nullptr, "Repeated synchronize leaked resource references");
    }
}

int main() {
    bgfx::Init config{};
    config.type = bgfx::RendererType::Noop;
    config.resolution.width = 1;
    config.resolution.height = 1;
    if (!bgfx::init(config)) {
        std::cerr << "Could not initialize the Noop renderer for binding tests\n";
        return 1;
    }
    const int failures = run("strict rollback, partial authoring bindings, save/open and explicit resource retry", bindingRecovery);
    bgfx::shutdown();
    return failures == 0 ? 0 : 1;
}
