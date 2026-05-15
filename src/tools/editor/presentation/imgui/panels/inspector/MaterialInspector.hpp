#pragma once
#include <glm/glm.hpp>
#include <imgui.h>
#include "../../../../business/EditorSelection.hpp"
#include "../../../../business/asset_editing/EditorAssetRepository.hpp"
#include "../../../../business/asset_editing/MaterialMutations.hpp"
#include "ComponentContainer.hpp"
#include "modules/asset_management/asset_types/Material.hpp"

class MaterialInspector : public ComponentContainer {
public:
    MaterialInspector(EditorAssetRepository& assetRepository,
                      EditorSelection& editorSelection,
                      MaterialMutations& materialMutations) :
        ComponentContainer("Material", 18),
        assetRepository_(assetRepository),
        editorSelection_(editorSelection),
        materialMutations_(materialMutations),
        colorPickerValue_{1.0f, 1.0f, 1.0f, 1.0f} {}

private:
    EditorAssetRepository& assetRepository_;
    EditorSelection& editorSelection_;
    MaterialMutations& materialMutations_;
    float colorPickerValue_[4];

    void drawBody() override {
        const auto& inspected = editorSelection_.getInspectedAsset();
        if (!inspected.has_value()) return;
        const AssetHandle& selectedAsset = *inspected;

        if (const Material* mat = assetRepository_.get<Material>(selectedAsset)) {
            bool isMutable = !materialMutations_.isMutable(selectedAsset);

            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 6));

            if (isMutable) {
                ImGui::BeginDisabled();
            }

            row("Name", [&] { ImGui::TextUnformatted(selectedAsset.c_str()); });
            row("Albedo", [&] {
                ImGui::SetNextItemWidth(-1);
                const std::string displayText = (mat->albedoTexture == EMPTY_TEXTURE) ? "" : mat->albedoTexture;
                ImGui::InputText("##albedoTexture",
                                 const_cast<char*>(displayText.c_str()),
                                 displayText.size() + 1,
                                 ImGuiInputTextFlags_ReadOnly);
                if (ImGui::BeginDragDropTarget()) {
                    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("texture_asset")) {
                        const char* textureName = static_cast<const char*>(payload->Data);
                        materialMutations_.setAlbedoTexture(selectedAsset, textureName);
                    }
                    ImGui::EndDragDropTarget();
                }
            });

            glm::vec4 color = mat->tint;
            colorPickerValue_[0] = color.r;
            colorPickerValue_[1] = color.g;
            colorPickerValue_[2] = color.b;
            colorPickerValue_[3] = color.a;
            if (ImGui::ColorEdit4("Tint", colorPickerValue_)) {
                materialMutations_.setTint(selectedAsset,
                                           glm::vec4(colorPickerValue_[0],
                                                     colorPickerValue_[1],
                                                     colorPickerValue_[2],
                                                     colorPickerValue_[3]));
            }

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            row("Normal Texture", [&] {
                ImGui::SetNextItemWidth(-1);
                const std::string displayText = (mat->normalTexture.empty()) ? "" : mat->normalTexture;
                ImGui::InputText("##normalTexture",
                                 const_cast<char*>(displayText.c_str()),
                                 displayText.size() + 1,
                                 ImGuiInputTextFlags_ReadOnly);
                if (ImGui::BeginDragDropTarget()) {
                    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("texture_asset")) {
                        const char* textureName = static_cast<const char*>(payload->Data);
                        materialMutations_.setNormalTexture(selectedAsset, textureName);
                    }
                    ImGui::EndDragDropTarget();
                }
            });

            row("Roughness Texture", [&] {
                ImGui::SetNextItemWidth(-1);
                const std::string displayText = (mat->roughnessTexture.empty()) ? "" : mat->roughnessTexture;
                ImGui::InputText("##roughnessTexture",
                                 const_cast<char*>(displayText.c_str()),
                                 displayText.size() + 1,
                                 ImGuiInputTextFlags_ReadOnly);
                if (ImGui::BeginDragDropTarget()) {
                    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("texture_asset")) {
                        const char* textureName = static_cast<const char*>(payload->Data);
                        materialMutations_.setRoughnessTexture(selectedAsset, textureName);
                    }
                    ImGui::EndDragDropTarget();
                }
            });

            row("Metallic Texture", [&] {
                ImGui::SetNextItemWidth(-1);
                const std::string displayText = (mat->metallicTexture.empty()) ? "" : mat->metallicTexture;
                ImGui::InputText("##metallicTexture",
                                 const_cast<char*>(displayText.c_str()),
                                 displayText.size() + 1,
                                 ImGuiInputTextFlags_ReadOnly);
                if (ImGui::BeginDragDropTarget()) {
                    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("texture_asset")) {
                        const char* textureName = static_cast<const char*>(payload->Data);
                        materialMutations_.setMetallicTexture(selectedAsset, textureName);
                    }
                    ImGui::EndDragDropTarget();
                }
            });

            row("AO Texture", [&] {
                ImGui::SetNextItemWidth(-1);
                const std::string displayText = (mat->aoTexture.empty()) ? "" : mat->aoTexture;
                ImGui::InputText("##aoTexture",
                                 const_cast<char*>(displayText.c_str()),
                                 displayText.size() + 1,
                                 ImGuiInputTextFlags_ReadOnly);
                if (ImGui::BeginDragDropTarget()) {
                    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("texture_asset")) {
                        const char* textureName = static_cast<const char*>(payload->Data);
                        materialMutations_.setAoTexture(selectedAsset, textureName);
                    }
                    ImGui::EndDragDropTarget();
                }
            });

            row("Metallic", [&] {
                float metallic = mat->metallic;
                if (ImGui::SliderFloat("##metallic", &metallic, 0.0f, 1.0f)) {
                    materialMutations_.setMetallic(selectedAsset, metallic);
                }
            });

            row("Roughness", [&] {
                float roughness = mat->roughness;
                if (ImGui::SliderFloat("##roughness", &roughness, 0.0f, 1.0f)) {
                    materialMutations_.setRoughness(selectedAsset, roughness);
                }
            });

            row("Ambient Occlusion", [&] {
                float ao = mat->ao;
                if (ImGui::SliderFloat("##ao", &ao, 0.0f, 1.0f)) {
                    materialMutations_.setAo(selectedAsset, ao);
                }
            });

            row("Scale Invariant UV", [&] {
                bool scaleInvariantUV = mat->scaleInvariantUV;
                if (ImGui::Checkbox("##scaleInvariantUV", &scaleInvariantUV)) {
                    materialMutations_.setScaleInvariantUV(selectedAsset, scaleInvariantUV ? 1 : 0);
                }
            });

            row("Tiling", [&] {
                glm::vec2 tiling = mat->tiling;
                ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.5f - 4.0f);
                bool changed = false;
                changed |= ImGui::InputFloat("##tilingX", &tiling.x, 0.0f, 0.0f, "%.3f");
                ImGui::SameLine(0, 4.0f);
                changed |= ImGui::InputFloat("##tilingY", &tiling.y, 0.0f, 0.0f, "%.3f");
                ImGui::PopItemWidth();
                if (changed) {
                    materialMutations_.setTiling(selectedAsset, tiling);
                }
            });
            row("Offset", [&] {
                glm::vec2 offset = mat->offset;
                ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.5f - 4.0f);
                bool changed = false;
                changed |= ImGui::InputFloat("##offsetX", &offset.x, 0.0f, 0.0f, "%.3f");
                ImGui::SameLine(0, 4.0f);
                changed |= ImGui::InputFloat("##offsetY", &offset.y, 0.0f, 0.0f, "%.3f");
                ImGui::PopItemWidth();
                if (changed) {
                    materialMutations_.setOffset(selectedAsset, offset);
                }
            });

            if (isMutable) {
                ImGui::EndDisabled();
                ImGui::Spacing();
                ImGui::TextDisabled("(Builtin materials cannot be edited)");
            }

            ImGui::PopStyleVar();
        }
    }

    template<typename Fn>
    static void row(const char* label, Fn&& fn) {
        ImGui::Columns(2, nullptr, false);
        ImGui::SetColumnWidth(0, 160.0f); // Increased width for fields column
        ImGui::Text("%s:", label);
        ImGui::NextColumn();
        fn();
        ImGui::Columns(1);
    }
};
