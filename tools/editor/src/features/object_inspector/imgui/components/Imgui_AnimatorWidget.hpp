#pragma once
#include <array>
#include <optional>
#include "../../../../services/common_editing/AssetStore.hpp"
#include "../../../../services/common_editing/RuntimeScene.hpp"
#include "../Imgui_InspectorWidget.hpp"
#include "modules/asset_management/asset_types/AnimatorController.hpp"
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

    void setComponent(const Animator& c, const Renderer* /*renderer*/) {
        component_ = c;
        auto src = c.controllerName;
        src.resize(std::min(src.size(), controllerBuf_.size() - 1));
        std::copy(src.begin(), src.end(), controllerBuf_.begin());
        controllerBuf_[src.size()] = '\0';
    }

private:
    RuntimeScene& scene_;
    AssetStore& assetStore_;
    std::optional<EntityHandle> lastObjectId_;
    Animator component_{};
    std::array<char, 256> controllerBuf_{};

    static std::string shortName(const std::string& full) {
        auto slash = full.rfind('/');
        auto dot = full.rfind('.');
        if (slash != std::string::npos && dot != std::string::npos && dot > slash)
            return full.substr(slash + 1, dot - slash - 1);
        return full;
    }

    void buildProperties() override {
        if (!lastObjectId_) return;

        // ---- Controller asset path ----
        add<InputTextEditableProperty>(
                "Controller",
                controllerBuf_,
                [this] {
                    component_.controllerName = controllerBuf_.data();
                    commit(UndoHistory::randomGroupId("Set Controller"));
                },
                "ANIMCTRL_ASSET",
                [this](const char* path) {
                    std::strncpy(controllerBuf_.data(), path, controllerBuf_.size() - 1);
                    controllerBuf_.back() = '\0';
                    component_.controllerName = controllerBuf_.data();
                    commit(UndoHistory::randomGroupId("Set Controller"));
                });

        // Try to load controller to show its states.
        const AnimatorController* controller = nullptr;
        if (!component_.controllerName.empty()) {
            try {
                controller = &assetStore_.getAsset<AnimatorController>(component_.controllerName);
            } catch (...) {
            }
        }

        if (!controller) {
            ImGui::TextDisabled("No controller loaded");
            return;
        }

        // ---- Active state + playback ----
        {
            std::vector<std::string> stateNames;
            stateNames.reserve(controller->states.size());
            for (const auto& [name, _]: controller->states)
                stateNames.push_back(name);

            if (!stateNames.empty()) {
                add<ComboProperty>(
                        "Active State",
                        stateNames,
                        [this, stateNames] {
                            for (int i = 0; i < static_cast<int>(stateNames.size()); ++i)
                                if (stateNames[i] == component_.currentState) return i;
                            return 0;
                        },
                        [this, stateNames](int idx) {
                            if (idx >= 0 && idx < static_cast<int>(stateNames.size()))
                                component_.currentState = stateNames[idx];
                            commit(UndoHistory::randomGroupId("Set Active State"));
                        });
            }

            add<CheckboxProperty>(
                    "Playing",
                    [this] { return component_.playing; },
                    [this](bool v) {
                        component_.playing = v;
                        commit(UndoHistory::randomGroupId("Set Playing"));
                    });
        }

        // ---- Read-only state list from controller ----
        ImGui::Separator();
        ImGui::TextUnformatted("States (from controller)");
        ImGui::Spacing();

        for (const auto& [stateName, state]: controller->states) {
            ImGui::PushID(stateName.c_str());
            if (ImGui::CollapsingHeader(stateName.c_str())) {
                ImGui::TextDisabled("Clip:  %s", shortName(state.clipName).c_str());
                ImGui::TextDisabled("Speed: %.2f", state.speed);
                ImGui::TextDisabled("Loop:  %s", state.loop ? "yes" : "no");
            }
            ImGui::PopID();
        }
    }

    void commit(const std::string& groupId) {
        if (!lastObjectId_) return;
        Animator snapshot = component_;
        scene_.getObject(*lastObjectId_).mutateComponent<Animator>([snapshot](Animator& a) { a = snapshot; }, groupId);
    }
};
