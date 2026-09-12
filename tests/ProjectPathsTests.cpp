#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <string_view>

#include "components/IdentityComponent.hpp"
#include "core/EngineConfig.hpp"
#include "systems/SceneSerializer.hpp"

namespace {
    auto expect(bool condition, std::string_view message, int& failures) -> void {
        if (!condition) {
            std::cerr << "FAIL: " << message << '\n';
            ++failures;
        }
    }

    auto write(const std::filesystem::path& path, std::string_view text) -> void {
        std::ofstream stream(path, std::ios::binary);
        stream << text;
        if (!stream) {
            throw std::runtime_error("Failed to write test fixture");
        }
    }

    auto read(const std::filesystem::path& path) -> std::string {
        std::ifstream stream(path, std::ios::binary);
        return {std::istreambuf_iterator<char>{stream}, std::istreambuf_iterator<char>{}};
    }

    struct TemporaryProject {
        std::filesystem::path previousDirectory = std::filesystem::current_path();
        std::filesystem::path root = std::filesystem::canonical(std::filesystem::temp_directory_path()) /
            ("aether project paths " + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));

        ~TemporaryProject() {
            std::error_code error;
            std::filesystem::current_path(previousDirectory, error);
            std::filesystem::remove_all(root, error);
        }
    };
}

int main() {
    using namespace AetherEngine;
    int failures = 0;
    TemporaryProject fixture;
    try {
        const auto projectRoot = fixture.root / "Authoring Project";
        const auto runtimeRoot = fixture.root / "Package";
        const auto templateRoot = fixture.root / "templates";
        for (const auto& root : {projectRoot, runtimeRoot}) {
            std::filesystem::create_directories(root / "assets/scenes");
            std::filesystem::create_directories(root / "assets/shaders");
        }
        std::filesystem::create_directories(templateRoot);
        constexpr std::string_view original =
            R"({"version":2,"name":"Main","entities":[{"id":"object","name":"Original","components":{}}]})";
        const auto workingScene = projectRoot / "assets/scenes/Main.scene.json";
        const auto packagedScene = runtimeRoot / "assets/scenes/Main.scene.json";
        const auto templateScene = templateRoot / "DefaultScene.scene.json";
        write(workingScene, original);
        write(packagedScene, original);
        write(templateScene, original);

        // Authoring is selected explicitly, even when a valid runtime package
        // exists and the current working directory points somewhere else.
        std::filesystem::current_path(runtimeRoot);
        const core::EngineInitConfig config{.projectRoot = projectRoot};
        const auto authoring = core::resolveEnginePaths(config, runtimeRoot);
        const auto runtime = core::resolveEnginePaths({}, runtimeRoot);
        expect(authoring.has_value() && runtime.has_value(), "resolve project and packaged paths", failures);
        // SDL_GetBasePath returns a trailing separator. Exercise the same path
        // shape as game startup, rather than only hand-built directory paths.
        const auto runtimeWithSeparator = core::resolveEnginePaths({}, runtimeRoot / "");
        expect(runtimeWithSeparator && runtimeWithSeparator->assetsRoot == runtimeRoot / "assets",
            "packaged startup accepts SDL-style trailing separator", failures);
        const auto projectWithSeparator = core::resolveEnginePaths({.projectRoot = projectRoot / ""}, runtimeRoot);
        expect(projectWithSeparator && projectWithSeparator->projectRoot == projectRoot,
            "project path accepts and normalizes trailing separator", failures);
#if defined(_WIN32)
        const auto runtimeWithBackslash = core::resolveEnginePaths({},
            std::filesystem::path{runtimeRoot.wstring() + L"\\"});
        expect(runtimeWithBackslash && runtimeWithBackslash->assetsRoot == runtimeRoot / "assets",
            "packaged startup accepts native Windows trailing separator", failures);
#endif
        if (authoring && runtime) {
            expect(authoring->projectRoot == projectRoot, "retain explicit project root", failures);
            expect(authoring->scenesRoot == projectRoot / "assets/scenes", "save into project scenes", failures);
            expect(runtime->projectRoot.empty(), "packaged game has no authoring project", failures);
            expect(runtime->assetsRoot == runtimeRoot / "assets", "game uses executable-relative assets", failures);
            expect(authoring->assetsRoot != runtime->assetsRoot, "authoring and runtime are separate", failures);

            auto serializer = systems::SceneSerializer::createSceneSerializer(authoring->scenesRoot);
            expect(static_cast<bool>(serializer), "create authoring serializer", failures);
            if (serializer) {
                auto loaded = serializer->deserializeScene("Main");
                expect(loaded.has_value(), "open working scene", failures);
                if (loaded) {
                    auto object = (*loaded)->findEntityById("object");
                    expect(object.has_value(), "find fixture object", failures);
                    if (object) {
                        (*loaded)->getComponent<components::IdentityComponent>(*object).name = "Author edit";
                        // Resolved paths remain valid if the working directory changes.
                        std::filesystem::current_path(templateRoot);
                        expect(serializer->serializeScene(**loaded).has_value(), "save author edit", failures);
                        auto reopened = serializer->deserializeScene("Main");
                        expect(reopened.has_value(), "reopen saved scene", failures);
                        if (reopened) {
                            auto restored = (*reopened)->findEntityById("object");
                            expect(restored && (*reopened)->getComponent<components::IdentityComponent>(*restored).name == "Author edit",
                                "author edit survives reopening", failures);
                        }
                    }
                }
            }
        }
        expect(read(workingScene).find("Author edit") != std::string::npos, "working file contains edit", failures);
        expect(read(templateScene) == original, "save preserves template", failures);
        expect(read(packagedScene) == original, "save preserves existing runtime package", failures);

        expect(core::resolveEnginePaths(config, {}).has_value(), "explicit project does not require a package", failures);
        expect(!core::resolveEnginePaths({.projectRoot = fixture.root / "Missing"}, runtimeRoot),
            "invalid project never falls back to packaged assets", failures);
        expect(!std::filesystem::exists(fixture.root / "Missing"), "opening does not create or seed a project", failures);
        expect(!core::resolveEnginePaths({.projectRoot = templateRoot}, runtimeRoot),
            "template folder is not an authoring project", failures);
        expect(!core::resolveEnginePaths({.projectRoot = projectRoot, .startupScene = "../Main"}, runtimeRoot),
            "reject scene path traversal", failures);
        expect(!core::resolveEnginePaths({}, fixture.root / "MissingPackage"),
            "packaged mode requires its own assets", failures);
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    if (failures == 0) {
        std::cout << "Project path and save isolation tests passed\n";
    }
    return failures == 0 ? 0 : 1;
}
