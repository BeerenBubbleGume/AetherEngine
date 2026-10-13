//
// Created by drhaz on 08.10.2026.
//

#ifndef SMB_DOCUMENTCONTROLLER_HPP
#define SMB_DOCUMENTCONTROLLER_HPP
#include <memory>

#include "SceneEditorDocument.hpp"
#include "core/EngineContext.hpp"


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
        auto saveAs(AetherEngine::core::EngineContext& ctx, std::string_view filename) -> std::expected<void, AetherEngine::core::EngineError>;
        auto save(AetherEngine::core::EngineContext& ctx) -> std::expected<void, AetherEngine::core::EngineError>;
        auto load(AetherEngine::core::EngineContext& ctx, std::string_view filename) -> std::expected<void, AetherEngine::core::EngineError>;
        auto newScene(AetherEngine::core::EngineContext& ctx, std::string_view name) -> std::expected<void, AetherEngine::core::EngineError>;

        auto dirty(bool flag) -> void;
        auto setFileStem(std::string_view filename) -> void;
        auto setScene(AetherEngine::scene::Scene& scene) -> void;

        static auto listScenes(const  AetherEngine::core::EngineContext& ctx) -> std::expected<std::vector<std::string>, AetherEngine::core::EngineError>;

        [[nodiscard]] auto getDocument() const -> const SceneEditorDocument&;
    private:
        DocumentController() = default;
        ~DocumentController() = default;
        docs::SceneEditorDocument m_document{};
    };
} // docs
// AetherEditor

#endif //SMB_DOCUMENTCONTROLLER_HPP
