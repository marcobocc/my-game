#pragma once
#include <array>
#include <filesystem>
#include <optional>
#include <string>
#include "../Imgui_InspectorWidget.hpp"
#include "modules/scene/components/BehaviourScript.hpp"

class RuntimeScene;

class Imgui_BehaviourScriptWidget : public Imgui_InspectorWidget {
public:
    Imgui_BehaviourScriptWidget(RuntimeScene& scene, std::string title) :
        Imgui_InspectorWidget(std::move(title)),
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

    void buildProperties() override {
        if (!lastObjectId_) return;

        add<InputTextEditableProperty>(
                "Script",
                nameBuf_,
                [this] {
                    component_.scriptName = nameBuf_.data();
                    commitEdit();
                },
                "SCRIPT_ASSET",
                [this](const char* path) {
                    std::string className = std::filesystem::path(path).stem().string();
                    if (!className.empty()) {
                        component_.scriptName = className;
                        component_.scriptName.copy(nameBuf_.data(), nameBuf_.size() - 1);
                        nameBuf_[component_.scriptName.size()] = '\0';
                        commitEdit();
                    }
                });
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
