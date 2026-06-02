#pragma once
#include <array>
#include <filesystem>
#include <imgui.h>
#include "../Imgui_InspectorWidget.hpp"
#include "modules/scene/components/BehaviourScript.hpp"

class RuntimeScene;

class Imgui_BehaviourScriptWidget : public Imgui_InspectorWidget {
public:
    explicit Imgui_BehaviourScriptWidget(RuntimeScene& scene) :
        Imgui_InspectorWidget("BehaviourScript", 3),
        scene_(scene) {}

    void setCurrentObjectId(EntityHandle objectId) { lastObjectId_.emplace(objectId); }

    void setComponent(const BehaviourScript& c) {
        component_ = c;
        // Keep the char buffer in sync with the string value
        component_.scriptName.copy(nameBuf_.data(), nameBuf_.size() - 1);
        nameBuf_[component_.scriptName.size()] = '\0';
    }

private:
    RuntimeScene& scene_;
    std::optional<EntityHandle> lastObjectId_;
    BehaviourScript component_{};
    std::array<char, 256> nameBuf_{};

    void drawBody() override {
        if (!lastObjectId_) return;

        ImGui::Columns(2, nullptr, false);
        ImGui::SetColumnWidth(0, 90.0f);
        ImGui::TextUnformatted("Script");
        ImGui::NextColumn();
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        if (ImGui::InputText("##scriptName", nameBuf_.data(), nameBuf_.size(), ImGuiInputTextFlags_EnterReturnsTrue)) {
            component_.scriptName = nameBuf_.data();
            commitEdit();
        }
        if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("SCRIPT_ASSET")) {
                std::string vfsPath = static_cast<const char*>(payload->Data);
                component_.scriptName = std::filesystem::path(vfsPath).filename().string();
                // sync buffer
                component_.scriptName.copy(nameBuf_.data(), nameBuf_.size() - 1);
                nameBuf_[component_.scriptName.size()] = '\0';
                commitEdit();
            }
            ImGui::EndDragDropTarget();
        }
        ImGui::Columns(1);
    }

    void commitEdit() {
        if (!lastObjectId_) return;
        BehaviourScript snapshot = component_;
        scene_.getObject(*lastObjectId_)
                .mutateComponent<BehaviourScript>([snapshot](BehaviourScript& b) { b = snapshot; },
                                                  UndoHistory::randomGroupId("Set Script Name"));
    }
};
