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
                                    std::string_view filename, bool overwriteConfirmed) -> std::expected<void, AetherEngine::core::EngineError> {
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

        // Allocate the new name before committing the file to keep the document
        // metadata unchanged if preparing Save As fails.
        std::string newFileStem{filename};
        auto serialized = ctx.sceneSerializer.serializeScene(*m_document.scene, filename,
            overwriteConfirmed ? AetherEngine::security::FileWriteMode::Replace
                               : AetherEngine::security::FileWriteMode::CreateNew);
        if (!serialized) {
            return std::unexpected(AetherEngine::core::EngineError{serialized.error().code, serialized.error().message});
        }
        m_document.fileStem = std::move(newFileStem);
        saved();
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
        saved();
        return {};
    }

    auto DocumentController::saved() -> void {
        m_document.dirty = false;
        if (m_actionState == ActionState::Saving) m_actionState = ActionState::Ready;
    }

    auto DocumentController::requestAction(core::PendingDocumentAction action) -> bool {
        if (m_pendingAction) return false;
        m_pendingAction = std::move(action);
        m_actionState = m_document.dirty ? ActionState::Confirmation : ActionState::Ready;
        return true;
    }

    auto DocumentController::decideAction(core::SceneLoadDecision decision) -> void {
        if (decision == core::SceneLoadDecision::Cancel) {
            cancelAction();
        } else if (m_actionState == ActionState::Confirmation) {
            if (decision == core::SceneLoadDecision::SaveChanges) m_actionState = ActionState::Saving;
            if (decision == core::SceneLoadDecision::DiscardChanges) m_actionState = ActionState::Ready;
        }
    }

    auto DocumentController::cancelAction() -> void {
        m_pendingAction.reset();
        m_actionState = ActionState::None;
    }

    auto DocumentController::needsConfirmation() const -> bool { return m_actionState == ActionState::Confirmation; }
    auto DocumentController::awaitingSave() const -> bool { return m_actionState == ActionState::Saving; }
    auto DocumentController::hasPendingAction() const -> bool { return m_pendingAction.has_value(); }

    auto DocumentController::takeReadyAction() -> std::optional<core::PendingDocumentAction> {
        if (m_actionState != ActionState::Ready) return std::nullopt;
        auto action = std::move(m_pendingAction);
        cancelAction();
        return action;
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
