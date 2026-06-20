#pragma once
#include <optional>
#include "../../../../services/common_editing/AssetStore.hpp"
#include "../../../../services/common_editing/RuntimeScene.hpp"
#include "../Imgui_InspectorWidget.hpp"
#include "modules/asset_management/asset_types/Mesh.hpp"
#include "modules/scene/components/Animator.hpp"
#include "modules/scene/components/Renderer.hpp"

class Imgui_AnimatorWidget : public Imgui_InspectorWidget {
public:
    Imgui_AnimatorWidget(RuntimeScene& scene, AssetStore& assetStore) :
        Imgui_InspectorWidget("Animator"),
        scene_(scene),
        assetStore_(assetStore) {}

    void setCurrentObjectId(EntityHandle objectId) { lastObjectId_.emplace(objectId); }

    void setComponent(const Animator& c, const Renderer* renderer) {
        component_ = c;
        meshName_ = renderer ? renderer->meshName : "";
    }

private:
    RuntimeScene& scene_;
    AssetStore& assetStore_;
    std::optional<EntityHandle> lastObjectId_;
    Animator component_{};
    std::string meshName_;

    std::vector<std::string> getClipNames() const {
        if (meshName_.empty()) return {};
        try {
            const Mesh& mesh = assetStore_.getAsset<Mesh>(meshName_);
            return mesh.getClipNames();
        } catch (...) {
            return component_.clipNames;
        }
    }

    void buildProperties() override {
        if (!lastObjectId_) return;

        const std::vector<std::string> clips = getClipNames();

        if (clips.empty()) {
            add<LabelProperty>("Clip", [] { return std::string("No clips (load mesh first)"); });
        } else {
            std::vector<std::string> displayNames;
            displayNames.reserve(clips.size());
            for (const auto& fullName: clips) {
                auto slash = fullName.rfind('/');
                auto dot = fullName.rfind('.');
                if (slash != std::string::npos && dot != std::string::npos && dot > slash)
                    displayNames.push_back(fullName.substr(slash + 1, dot - slash - 1));
                else
                    displayNames.push_back(fullName);
            }

            add<ComboProperty>(
                    "Clip",
                    displayNames,
                    [this, clips] {
                        for (int i = 0; i < static_cast<int>(clips.size()); ++i)
                            if (clips[i] == component_.currentClip) return i;
                        return 0;
                    },
                    [this, clips](int idx) {
                        if (idx >= 0 && idx < static_cast<int>(clips.size())) component_.currentClip = clips[idx];
                        commit(UndoHistory::randomGroupId("Set Clip"));
                    });
        }

        add<DragFloatProperty>(
                "Speed",
                [this] { return component_.speed; },
                [this](float v) { component_.speed = v; },
                0.05f,
                0.0f,
                10.0f,
                [this] { commit(UndoHistory::randomGroupId("Set Speed")); });

        add<CheckboxProperty>(
                "Loop",
                [this] { return component_.loop; },
                [this](bool v) {
                    component_.loop = v;
                    commit(UndoHistory::randomGroupId("Set Loop"));
                });

        add<CheckboxProperty>(
                "Playing",
                [this] { return component_.playing; },
                [this](bool v) {
                    component_.playing = v;
                    commit(UndoHistory::randomGroupId("Set Playing"));
                });
    }

    void commit(const std::string& groupId) {
        if (!lastObjectId_) return;
        Animator snapshot = component_;
        scene_.getObject(*lastObjectId_).mutateComponent<Animator>([snapshot](Animator& a) { a = snapshot; }, groupId);
    }
};
