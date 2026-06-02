#pragma once
#include <array>
#include <filesystem>
#include <imgui.h>
#include <string>
#include "../Imgui_InspectorWidget.hpp"
#include "modules/scene/components/BehaviourScript.hpp"

class RuntimeScene;

class Imgui_BehaviourScriptWidget : public Imgui_InspectorWidget {
public:
    Imgui_BehaviourScriptWidget(RuntimeScene& scene, std::string title) :
        Imgui_InspectorWidget(std::move(title), 3),
        scene_(scene) {}

    void setCurrentObjectId(EntityHandle objectId) { lastObjectId_.emplace(objectId); }

    void setComponent(const BehaviourScript& c, size_t index) {
        index_ = index;
        component_ = c;
        component_.scriptName.copy(nameBuf_.data(), nameBuf_.size() - 1);
        nameBuf_[component_.scriptName.size()] = '\0';
    }

private:
    RuntimeScene& scene_;
    std::optional<EntityHandle> lastObjectId_;
    BehaviourScript component_{};
    std::array<char, 256> nameBuf_{};
    size_t index_ = 0;

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
                std::string className = extractClassName(vfsPath);
                if (!className.empty()) {
                    component_.scriptName = className;
                    component_.scriptName.copy(nameBuf_.data(), nameBuf_.size() - 1);
                    nameBuf_[component_.scriptName.size()] = '\0';
                    commitEdit();
                }
            }
            ImGui::EndDragDropTarget();
        }
        ImGui::Columns(1);
    }

    static std::string extractClassName(const std::string& vfsPath) {
        return std::filesystem::path(vfsPath).stem().string();
    }

    void commitEdit() {
        if (!lastObjectId_) return;
        BehaviourScript snapshot = component_;
        size_t index = index_;
        scene_.getObject(*lastObjectId_)
                .mutateComponentAt<BehaviourScript>(
                        index,
                        [snapshot](BehaviourScript& b) { b = snapshot; },
                        UndoHistory::randomGroupId("Set Script Name"));
    }
};
