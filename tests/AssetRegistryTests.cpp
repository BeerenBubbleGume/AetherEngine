#include <filesystem>
#include <iostream>
#include <string_view>

#include <bgfx/bgfx.h>

#include "assets/AssetManager.hpp"
#include "assets/AssetRegistry.hpp"
#include "resources/ResourceManager.hpp"

namespace {
    auto expect(bool condition, std::string_view message, int& failures) -> void {
        if (!condition) {
            std::cerr << "FAIL: " << message << '\n';
            ++failures;
        }
    }
}

int main() {
    using namespace AetherEngine::assets;

    int failures = 0;
    AssetRegistry registry{std::filesystem::path{AETHER_TEST_ASSETS_ROOT}};
    const auto loaded = registry.loadManifest("registry/runtime-assets.json");
    expect(loaded.has_value(), "load runtime asset manifest", failures);
    if (!loaded) {
        std::cerr << loaded.error().message << '\n';
        return 1;
    }

    const auto bunny = registry.findByAlias("asset://meshes/bunny");
    expect(bunny && bunny->type == AssetType::Mesh, "resolve mesh alias", failures);
    const auto brick = registry.findByAlias("asset://materials/brick");
    expect(brick && brick->type == AssetType::Material, "resolve material alias", failures);

    const auto shaderId = parseAssetId("33333333333333333333333333333333");
    expect(shaderId.has_value(), "parse stable asset id", failures);
    if (shaderId) {
        expect(toString(*shaderId) == "33333333333333333333333333333333", "asset id round trip", failures);

        const auto windowsVariant = registry.resolveVariant(*shaderId, {
            OperatingSystem::Windows,
            Architecture::X64,
            GraphicsBackend::Direct3D11
        });
        expect(windowsVariant.has_value(), "select Windows D3D11 shader variant", failures);
        if (windowsVariant) {
            const auto vertex = windowsVariant->artifacts.find("vertex");
            expect(
                vertex != windowsVariant->artifacts.end() &&
                    vertex->second.generic_string() == "shaders/bin/win32/basic_vs.bin",
                "resolve Windows vertex shader artifact",
                failures
            );
        }

        const auto unsupportedVariant = registry.resolveVariant(*shaderId, {
            OperatingSystem::Linux,
            Architecture::X64,
            GraphicsBackend::Vulkan
        });
        expect(!unsupportedVariant, "reject missing Linux Vulkan shader variant", failures);
    }

    if (bunny) {
        const auto portableVariant = registry.resolveVariant(bunny->id, {
            OperatingSystem::Linux,
            Architecture::Arm64,
            GraphicsBackend::Vulkan
        });
        expect(portableVariant.has_value(), "select portable mesh variant", failures);
    }

    bgfx::Init init{};
    init.type = bgfx::RendererType::Noop;
    init.resolution.width = 1;
    init.resolution.height = 1;
    if (bgfx::init(init)) {
        auto resources = AetherEngine::resources::ResourceManager::createResourceManager(
            std::filesystem::path{AETHER_TEST_ASSETS_ROOT}
        );
        expect(static_cast<bool>(resources), "create low-level resource pool", failures);
        if (resources) {
            auto managerResult = AssetManager::create(
                std::filesystem::path{AETHER_TEST_ASSETS_ROOT},
                *resources,
                {OperatingSystem::Linux, Architecture::X64, GraphicsBackend::Vulkan}
            );
            expect(managerResult.has_value(), "create runtime asset manager", failures);
            if (managerResult) {
                auto manager = std::move(*managerResult);
                const auto reference = manager->reference<RuntimeMeshAsset>("asset://meshes/bunny");
                expect(reference.has_value(), "create typed mesh reference", failures);
                if (reference) {
                    const auto first = manager->load(*reference);
                    const auto second = manager->load(*reference);
                    expect(first.has_value() && second.has_value(), "deduplicate runtime mesh load", failures);
                    if (first && second) {
                        expect(*first == *second, "reuse typed runtime handle", failures);
                        expect(manager->get(*first) != nullptr, "resolve ready runtime mesh", failures);
                        manager->unload(reference->id);
                        expect(manager->get(*first) != nullptr, "retain multiply referenced runtime mesh", failures);
                        manager->unload(reference->id);
                        expect(manager->get(*first) == nullptr, "invalidate handle after final unload", failures);

                        const auto reloaded = manager->load(*reference);
                        expect(reloaded.has_value(), "reload an unloaded runtime mesh", failures);
                        if (reloaded) {
                            expect(reloaded->generation != first->generation, "advance handle generation", failures);
                            expect(manager->get(*reloaded) != nullptr, "resolve reloaded runtime mesh", failures);
                        }
                    }
                }
            }
        }
        resources.reset();
        bgfx::shutdown();
    } else {
        expect(false, "initialize bgfx Noop for runtime asset test", failures);
    }

    if (failures != 0) {
        std::cerr << failures << " asset registry test(s) failed\n";
        return 1;
    }
    std::cout << "All asset registry tests passed\n";
    return 0;
}
