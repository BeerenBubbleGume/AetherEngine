//
// Created by drhaz on 08.10.2026.
//

#include "documents/DocumentController.hpp"
#include <algorithm>
#include <string_view>
#include <utility>

#include "security/PathSecurity.hpp"

namespace AetherEditor::docs {
    auto DocumentController::createDocumentController() -> DocumentControllerPtr {
        return DocumentControllerPtr(new DocumentController(), DocumentControllerDeleter());
    }

    auto DocumentController::saveAs(AetherEngine::core::EngineContext &ctx,
                                    std::string_view filename) -> std::expected<void, AetherEngine::core::EngineError> {
        if (!m_document.scene) {
            return std::unexpected(
                AetherEngine::core::EngineError{2, "No scene to save"}
            );
        }
        if (filename.empty()) {
            return std::unexpected(
                AetherEngine::core::EngineError{2, "Scene file name is empty"}
            );
        }

        auto serialized = ctx.sceneSerializer.serializeScene(*m_document.scene, filename);
        if (!serialized) {
            return std::unexpected(AetherEngine::core::EngineError{2, serialized.error().message});
        }
        m_document.fileStem = filename;
        m_document.dirty = false;
        return {};
    }

    auto DocumentController::save(
        AetherEngine::core::EngineContext &ctx) -> std::expected<void, AetherEngine::core::EngineError> {
        if (m_document.fileStem.empty()) {
            return std::unexpected(
                AetherEngine::core::EngineError{
                    2, "Scene file name is empty"
                }
            );
        }
        if (!m_document.scene) {
            return std::unexpected(
                AetherEngine::core::EngineError{2, "No scene to save"}
                );
        }
        auto serialized = ctx.sceneSerializer.serializeScene(*m_document.scene, m_document.fileStem);
        if (!serialized) {
            return std::unexpected(AetherEngine::core::EngineError{2, serialized.error().message});
        }
        m_document.dirty = false;
        return {};
    }

    auto DocumentController::load(AetherEngine::core::EngineContext &ctx,
        std::string_view filename) -> std::expected<void, AetherEngine::core::EngineError> {
        auto loaded = ctx.sceneSerializer.deserializeScene(filename);
        if (!loaded) {
            return std::unexpected(
                AetherEngine::core::EngineError{2, loaded.error().message}
            );
        }
        auto replaced = ctx.replaceScene(std::move(*loaded));
        if (!replaced) {
            return std::unexpected(replaced.error());
        }

        m_document.scene = &ctx.getScene();
        m_document.fileStem = filename;
        m_document.dirty = false;
        return {};
    }

    auto DocumentController::newScene(AetherEngine::core::EngineContext &ctx,
        std::string_view name) -> std::expected<void, AetherEngine::core::EngineError> {
        auto createdScene = AetherEngine::scene::Scene::createScene(name);
        auto replaced = ctx.replaceScene(std::move(createdScene));
        if (!replaced) {
            return std::unexpected(replaced.error());
        }
        m_document.scene = &ctx.getScene();
        m_document.fileStem.clear();
        m_document.dirty = true;
        return {};
    }

    auto DocumentController::dirty(bool flag) -> void {
        m_document.dirty = flag;
    }

    auto DocumentController::setFileStem(std::string_view filename) -> void {
        m_document.fileStem = filename;
    }

    auto DocumentController::setScene(AetherEngine::scene::Scene &scene) -> void {
        m_document.scene = &scene;
    }

    auto DocumentController::listScenes(
        const AetherEngine::core::EngineContext &ctx) -> std::expected<std::vector<std::string>, AetherEngine::
        core::EngineError> {
        std::vector<std::string> scenes;
        constexpr std::string_view suffix = ".scene.json";

        try {
            for (const auto& entry :
                 std::filesystem::directory_iterator(ctx.paths.scenesRoot)) {
                if (!entry.is_regular_file()) {
                    continue;
                }

                const auto utf8 = entry.path().filename().u8string();
                std::string name{utf8.begin(), utf8.end()};

                if (!name.ends_with(suffix)) {
                    continue;
                }

                name.resize(name.size() - suffix.size());

                if (!AetherEngine::security::isSafeSceneName(name)) {
                    continue;
                }

                scenes.push_back(std::move(name));
                 }
        } catch (const std::filesystem::filesystem_error& error) {
            return std::unexpected(
                AetherEngine::core::EngineError{2, error.what()}
            );
        }

        std::sort(scenes.begin(), scenes.end());
        return scenes;
    }

    auto DocumentController::getDocument() const -> const SceneEditorDocument & {
        return m_document;
    }
} // docs
// AetherEditor