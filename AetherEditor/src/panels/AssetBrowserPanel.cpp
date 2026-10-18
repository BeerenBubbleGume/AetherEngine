//
// Created by drhaz on 08.10.2026.
//

#include "panels/AssetBrowserPanel.hpp"
#include <algorithm>
#include <utility>
#include <imgui.h>
#include <imgui_stdlib.h>


namespace AetherEditor::panels {
    auto AssetBrowserPanel::createAssetBrowserPanel() -> AssetBrowserPanelPtr {
        return AssetBrowserPanelPtr(new AssetBrowserPanel(), AssetBrowserPanelDeleter{});
    }

    auto AssetBrowserPanel::draw(bool &open, const std::vector<AetherEngine::assets::AssetRecord> &assetList) -> AssetBrowserFrame {
        AssetBrowserFrame frame;
        if (ImGui::Begin("Asset Browser", &open)) {
            frame.scanTexturesRequested = ImGui::Button("Refresh KTX");
            ImGui::SameLine();
            if (ImGui::Button("New Material...")) {
                m_openMaterialPopup = true;
                m_closeMaterialPopup = false;
                m_materialName.clear();
                m_status.clear();
                m_statusError = false;
                const auto selected = std::ranges::find(assetList, m_selectedAsset.value_or(AetherEngine::assets::AssetId{}),
                    &AetherEngine::assets::AssetRecord::id);
                if (selected != assetList.end() && selected->type == AssetType::Texture) m_materialTexture = selected->id;
            }
            if (!m_status.empty()) {
                if (m_statusError) ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{1.0f, 0.45f, 0.4f, 1.0f});
                ImGui::TextWrapped("%s", m_status.c_str());
                if (m_statusError) ImGui::PopStyleColor();
            }
            ImGui::Separator();
            for (const auto& group : groups) {
                if (!ImGui::CollapsingHeader(group.label)) {
                    continue;
                }

                ImGui::Indent();

                for (const auto& asset : assetList) {
                    if (asset.type != group.type) {
                        continue;
                    }

                    const auto id = AetherEngine::assets::toString(asset.id);
                    ImGui::PushID(id.c_str());

                    const bool selected = m_selectedAsset && *m_selectedAsset == asset.id;

                    const auto separator = asset.alias.find_last_of('/');
                    const auto offset = separator == std::string::npos ? 0 : separator + 1;

                    const char* displayName = asset.alias.c_str() + offset;

                    if (ImGui::Selectable(displayName, selected)) {
                        m_selectedAsset = asset.id;
                    }

                    if (ImGui::IsItemHovered()) {
                        ImGui::SetTooltip("%s", asset.alias.c_str());
                    }

                    ImGui::PopID();
                }

                ImGui::Unindent();
            }
            if (m_selectedAsset) {
                const auto it = std::ranges::find(
                    assetList,
                    *m_selectedAsset,
                    &AetherEngine::assets::AssetRecord::id
                );

                if (it != assetList.end()) {
                    drawAssetDetails(*it);
                } else {
                    m_selectedAsset.reset();
                }
            } else {
                ImGui::TextUnformatted("Select an asset");
            }
            drawCreateMaterial(assetList, frame);
        }
        ImGui::End();
        return frame;
    }

    auto AssetBrowserPanel::setStatus(std::string message, bool error) -> void {
        m_status = std::move(message);
        m_statusError = error;
    }

    auto AssetBrowserPanel::materialCreated(AetherEngine::assets::AssetId id) -> void {
        m_selectedAsset = id;
        m_closeMaterialPopup = true;
    }

    auto AssetBrowserPanel::drawCreateMaterial(
        const std::vector<AetherEngine::assets::AssetRecord>& assetList, AssetBrowserFrame& frame
    ) -> void {
        if (m_openMaterialPopup) {
            ImGui::OpenPopup("Create material");
            m_openMaterialPopup = false;
        }
        if (ImGui::BeginPopupModal("Create material", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            if (m_closeMaterialPopup) {
                ImGui::CloseCurrentPopup();
            } else {
                ImGui::InputText("Name", &m_materialName);
                const auto picker = [&](const char* label, AssetType type, std::optional<AetherEngine::assets::AssetId>& selection) {
                    auto current = std::ranges::find_if(assetList, [&](const auto& asset) {
                        return asset.type == type && selection && asset.id == *selection;
                    });
                    if (current == assetList.end()) {
                        current = std::ranges::find_if(assetList, [&](const auto& asset) { return asset.type == type; });
                        if (current == assetList.end()) selection.reset();
                        else selection = current->id;
                    }
                    const char* preview = current != assetList.end() ? current->alias.c_str() : "None available";
                    if (ImGui::BeginCombo(label, preview)) {
                        for (const auto& asset : assetList) {
                            if (asset.type != type) continue;
                            const auto id = AetherEngine::assets::toString(asset.id);
                            ImGui::PushID(id.c_str());
                            const bool selected = selection && *selection == asset.id;
                            if (ImGui::Selectable(asset.alias.c_str(), selected)) selection = asset.id;
                            if (selected) ImGui::SetItemDefaultFocus();
                            ImGui::PopID();
                        }
                        ImGui::EndCombo();
                    }
                };
                picker("Shader", AssetType::Shader, m_materialShader);
                picker("Texture", AssetType::Texture, m_materialTexture);
                if (m_statusError && !m_status.empty()) ImGui::TextWrapped("%s", m_status.c_str());
                ImGui::BeginDisabled(m_materialName.empty() || !m_materialShader || !m_materialTexture);
                if (ImGui::Button("Create")) {
                    frame.createMaterial = assets::CreateMaterialRequest{m_materialName, *m_materialShader, *m_materialTexture};
                }
                ImGui::EndDisabled();
                ImGui::SameLine();
                if (ImGui::Button("Cancel")) ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
        m_closeMaterialPopup = false;
    }

    auto AssetBrowserPanel::drawAssetDetails(const AetherEngine::assets::AssetRecord &asset) -> void {
        const char* typeLabel = "Unknown";

        for (const auto& group : groups) {
            if (group.type == asset.type) {
                typeLabel = group.label;
                break;
            }
        }

        ImGui::Separator();
        ImGui::TextUnformatted("Selected asset");

        ImGui::TextWrapped(
            "Asset ID: %s",
            AetherEngine::assets::toString(asset.id).c_str()
        );
        ImGui::Text("Type: %s", typeLabel);
        ImGui::TextWrapped("Alias: %s", asset.alias.c_str());
    }
} // panels
// AetherEditor
