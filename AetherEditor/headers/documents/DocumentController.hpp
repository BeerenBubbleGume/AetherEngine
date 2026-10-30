//
// Created by drhaz on 08.10.2026.
//

#ifndef SMB_DOCUMENTCONTROLLER_HPP
#define SMB_DOCUMENTCONTROLLER_HPP
#include <memory>

#include "SceneEditorDocument.hpp"
#include "core/EngineContext.hpp"
#include "core/ETypes.hpp"


namespace AetherEditor::docs {
    class DocumentController {
    public:
        struct DocumentControllerDeleter {
            void operator()(DocumentController* controller) const {
                delete controller;
            }
        };
        using DocumentControllerPtr = std::unique_ptr<DocumentController, DocumentControllerDeleter>;
        DocumentController(const DocumentController&) = delete;
        DocumentController& operator=(const DocumentController&) = delete;

        [[nodiscard]] static auto createDocumentController() -> DocumentControllerPtr;
        auto saveAs(AetherEngine::core::EngineContext& ctx, std::string_view filename, bool overwriteConfirmed = false) -> std::expected<void, AetherEngine::core::EngineError>;
        auto save(AetherEngine::core::EngineContext& ctx) -> std::expected<void, AetherEngine::core::EngineError>;
        auto load(AetherEngine::core::EngineContext& ctx, std::string_view filename) -> std::expected<void, AetherEngine::core::EngineError>;
        auto newScene(AetherEngine::core::EngineContext& ctx, std::string_view name) -> std::expected<void, AetherEngine::core::EngineError>;

        auto dirty(bool flag) -> void;
        auto setFileStem(std::string_view filename) -> void;
        auto setScene(AetherEngine::scene::Scene& scene) -> void;

        // A transition becomes executable only after discard, a successful save,
        // or an initially clean document. Failed/cancelled saves never release it.
        auto requestAction(core::PendingDocumentAction action) -> bool;
        auto decideAction(core::SceneLoadDecision decision) -> void;
        auto cancelAction() -> void;
        [[nodiscard]] auto needsConfirmation() const -> bool;
        [[nodiscard]] auto awaitingSave() const -> bool;
        [[nodiscard]] auto hasPendingAction() const -> bool;
        [[nodiscard]] auto takeReadyAction() -> std::optional<core::PendingDocumentAction>;

        static auto listScenes(const  AetherEngine::core::EngineContext& ctx) -> std::expected<std::vector<std::string>, AetherEngine::core::EngineError>;

        [[nodiscard]] auto getDocument() const -> const SceneEditorDocument&;
    private:
        DocumentController() = default;
        ~DocumentController() = default;
        docs::SceneEditorDocument m_document{};
        enum class ActionState { None, Confirmation, Saving, Ready };
        ActionState m_actionState = ActionState::None;
        std::optional<core::PendingDocumentAction> m_pendingAction;
        auto saved() -> void;
    };
} // docs
// AetherEditor

#endif //SMB_DOCUMENTCONTROLLER_HPP
