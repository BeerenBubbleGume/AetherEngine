#include "SceneWriteTestSupport.hpp"
#include "assets/AssetCatalogService.hpp"
#include "assets/AssetManager.hpp"
#include "resources/ResourceManager.hpp"
#include <nlohmann/json.hpp>
#include <bgfx/bgfx.h>
#include <SDL3/SDL.h>

namespace {
    namespace fs = std::filesystem;
    namespace assets = AetherEngine::assets;
    namespace editor = AetherEditor::assets;
    using namespace scene_write_tests;
    using Json = nlohmann::json;

    struct CatalogFixture {
        Fixture files;
        fs::path root = files.root / "assets";
        fs::path manifest = root / "registry/runtime-assets.json";
        AetherEngine::resources::ResourceManager::ResourceManagerPtr resources;
        std::unique_ptr<assets::AssetManager> manager;

        CatalogFixture() {
            fs::create_directories(root / "registry");
            fs::create_directories(root / "materials");
            fs::create_directories(root / "textures/bin/nested");
            // Keep the fixture independent of additional primitives and user assets
            // registered in Sandbox: these scenarios start with four known records.
            const auto source = Json::parse(get(fs::path{AETHER_TEST_ASSETS_ROOT} / "registry/runtime-assets.json"));
            Json catalog{{"version", 1}, {"assets", Json::array()}};
            for (const auto& record : source.at("assets")) {
                const auto alias = record.at("alias").get<std::string>();
                if (alias == "asset://meshes/bunny" || alias == "asset://shaders/basic" ||
                    alias == "asset://textures/brick" || alias == "asset://materials/brick") {
                    catalog["assets"].push_back(record);
                }
            }
            put(manifest, catalog.dump());
            copyTexture("brick.ktx");
            resources = AetherEngine::resources::ResourceManager::createResourceManager(root);
            restart();
        }
        void restart() {
            auto created = assets::AssetManager::create(root, *resources,
                {assets::OperatingSystem::Windows, assets::Architecture::X64, assets::GraphicsBackend::Direct3D11});
            require(created.has_value(), "Create asset manager");
            manager = std::move(*created);
        }
        void copyTexture(const fs::path& name) {
            fs::copy_file(fs::path{AETHER_TEST_ASSETS_ROOT} / "textures/bin/brick.ktx", root / "textures/bin" / name);
        }
        auto request(std::string name) -> editor::CreateMaterialRequest {
            return {std::move(name), manager->registry().findByAlias("asset://shaders/basic")->id,
                manager->registry().findByAlias("asset://textures/brick")->id};
        }
        void blockManifest() {
            fs::rename(manifest, root / "registry/backup.json");
            fs::create_directory(manifest);
        }
        void restoreManifest() {
            fs::remove(manifest);
            fs::rename(root / "registry/backup.json", manifest);
        }
    };

    void scanAndRestart() {
        CatalogFixture f;
        const auto originalId = f.manager->registry().findByAlias("asset://textures/brick")->id;
        auto handle = f.manager->load(assets::AssetRef<assets::RuntimeTextureAsset>{originalId});
        require(handle.has_value(), "Load existing texture before scan");
        const auto* object = f.manager->get(*handle);
        f.copyTexture("nested/extra.KTX");
        f.copyTexture(fs::path{u8"\u0442\u0435\u0441\u0442.ktx"});
        put(f.root / "textures/bin/broken.ktx", "not a KTX");
        put(f.root / "textures/bin/ignored.png", "not imported");
        auto report = editor::AssetCatalogService::scanTextures(*f.manager);
        require(report && report->added == 2 && report->existing == 1 && report->warnings.size() == 1,
            "Register only valid new KTX files");
        require(f.manager->get(*handle) == object, "Scan invalidated a loaded runtime handle");
        const auto* extra = f.manager->registry().findByAlias("asset://textures/nested/extra");
        require(extra != nullptr, "Nested KTX alias");
        const auto id = extra->id;
        const auto persisted = get(f.manifest);
        report = editor::AssetCatalogService::scanTextures(*f.manager);
        require(report && report->added == 0 && report->existing == 3 && get(f.manifest) == persisted,
            "Repeated scan must not duplicate records or rewrite catalog");
        f.restart();
        require(f.manager->registry().findByAlias("asset://textures/nested/extra")->id == id, "KTX ID changed after restart");
        const auto unicodeId = f.manager->registry().findByAlias("asset://textures/\xd1\x82\xd0\xb5\xd1\x81\xd1\x82")->id;
        auto loaded = f.manager->load(assets::AssetRef<assets::RuntimeTextureAsset>{unicodeId});
        require(loaded && f.manager->get(*loaded), "Load scanned Unicode KTX through runtime manager");
    }

    void createAndRestart() {
        CatalogFixture f;
        const auto request = f.request("New material");
        const auto created = editor::AssetCatalogService::createMaterial(*f.manager, request);
        require(created.has_value(), "Create material");
        const auto* record = f.manager->registry().find(*created);
        require(record && record->type == assets::AssetType::Material && record->dependencies.size() == 2,
            "Material record and dependencies");
        const auto materialPath = f.root / "materials/New material.material.json";
        const auto body = Json::parse(get(materialPath));
        require(body.at("shader") == assets::toString(request.shader) &&
                body.at("textures").at(0) == assets::toString(request.texture), "Material file references");
        const auto variant = f.manager->registry().resolveVariant(*created, f.manager->platform());
        require(variant.has_value(), "Resolve new material variant");
        f.restart();
        const auto reference = f.manager->reference<assets::RuntimeMaterialAsset>("asset://materials/New material");
        require(reference && reference->id == *created, "Material ID survives manager restart");
        const auto before = get(materialPath);
        const auto manifest = get(f.manifest);
        require(!editor::AssetCatalogService::createMaterial(*f.manager, request), "Reject duplicate material");
        require(get(materialPath) == before && get(f.manifest) == manifest, "Duplicate changed existing files");
        auto invalid = request;
        invalid.name = "../escape";
        require(!editor::AssetCatalogService::createMaterial(*f.manager, invalid), "Reject invalid material name");
        invalid = request;
        invalid.name = "Wrong type";
        invalid.texture = request.shader;
        require(!editor::AssetCatalogService::createMaterial(*f.manager, invalid), "Reject wrong texture type");
        put(f.root / "materials/Existing.material.json", "user contents");
        invalid = request;
        invalid.name = "Existing";
        require(!editor::AssetCatalogService::createMaterial(*f.manager, invalid), "Reject existing material file");
        require(get(f.root / "materials/Existing.material.json") == "user contents", "Overwrote user material");
    }

    void catalogWriteFailure() {
        CatalogFixture f;
        const auto count = f.manager->registry().listAssets().size();
        f.copyTexture("new.ktx");
        f.blockManifest();
        require(!editor::AssetCatalogService::scanTextures(*f.manager), "Catalog write failure was ignored");
        require(f.manager->registry().listAssets().size() == count, "Failed scan changed runtime catalog");
        f.restoreManifest();
        const auto report = editor::AssetCatalogService::scanTextures(*f.manager);
        require(report && report->added == 1, "Scan could not retry after catalog write failure");
        const auto request = f.request("Retry");
        f.blockManifest();
        require(!editor::AssetCatalogService::createMaterial(*f.manager, request), "Material catalog failure ignored");
        require(!f.manager->registry().findByAlias("asset://materials/Retry"), "Failed material entered runtime catalog");
        const auto path = f.root / "materials/Retry.material.json";
        const auto body = get(path);
        const auto id = *assets::parseAssetId(Json::parse(body).at("assetId").get<std::string>());
        f.restoreManifest();
        const auto created = editor::AssetCatalogService::createMaterial(*f.manager, request);
        require(created && *created == id && get(path) == body, "Retry replaced material or changed its ID");
    }

    void rejectInvalidBatch() {
        CatalogFixture f;
        const auto manifest = get(f.manifest);
        auto duplicate = *f.manager->registry().findByAlias("asset://textures/brick");
        require(!f.manager->registerAssets(std::span{&duplicate, 1}), "Duplicate catalog record accepted");
        auto invalid = duplicate;
        invalid.id = *assets::parseAssetId("99999999999999999999999999999999");
        invalid.alias = "asset://textures/invalid";
        invalid.variants[0].cookedPath = "../outside.ktx";
        require(!f.manager->registerAssets(std::span{&invalid, 1}), "Invalid record path accepted");
        require(get(f.manifest) == manifest && f.manager->registry().listAssets().size() == 4,
            "Invalid batch changed catalog");
    }

    void loadCreatedMaterial() {
        CatalogFixture f;
        f.copyTexture("new.ktx");
        require(editor::AssetCatalogService::scanTextures(*f.manager).has_value(), "Scan before material load");
        auto request = f.request("Runtime");
        request.texture = f.manager->registry().findByAlias("asset://textures/new")->id;
        const auto created = editor::AssetCatalogService::createMaterial(*f.manager, request);
        require(created.has_value(), "Create runtime material");
        const auto handle = f.manager->load(assets::AssetRef<assets::RuntimeMaterialAsset>{*created});
        require(handle.has_value(), "Load created material with real shader and scanned texture");
        const auto* material = f.manager->get(*handle);
        require(material && material->textures().size() == 1, "Loaded material texture binding");
        const auto* texture = f.manager->get(material->textures().front());
        const auto* shader = f.manager->get(material->shader());
        require(texture && texture->texture()->isValid() && shader && shader->program(), "Material runtime dependencies");
    }
}

int main(int argc, char** argv) {
    SDL_Window* window = nullptr;
    const bool graphics = argc == 2 && std::string_view{argv[1]} == "--d3d11";
    bgfx::Init init;
    init.type = bgfx::RendererType::Noop;
    init.resolution.width = 1;
    init.resolution.height = 1;
#ifdef _WIN32
    if (graphics) {
        if (!SDL_Init(SDL_INIT_VIDEO)) return 1;
        window = SDL_CreateWindow("Asset catalog runtime test", 32, 32, SDL_WINDOW_HIDDEN);
        if (!window) return 1;
        init.type = bgfx::RendererType::Direct3D11;
        init.platformData.nwh = SDL_GetPointerProperty(SDL_GetWindowProperties(window), SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);
    }
#endif
    if (!bgfx::init(init)) return 1;
    int failures = 0;
    failures += run("KTX scan, stable IDs, runtime handles and restart", scanAndRestart);
    failures += run("material creation, persistence and collision handling", createAndRestart);
    failures += run("catalog write failure and retry", catalogWriteFailure);
    failures += run("reject invalid catalog batches", rejectInvalidBatch);
    if (graphics) failures += run("load created material with D3D11", loadCreatedMaterial);
    bgfx::shutdown();
    if (window) SDL_DestroyWindow(window);
    if (graphics) SDL_Quit();
    return failures == 0 ? 0 : 1;
}
