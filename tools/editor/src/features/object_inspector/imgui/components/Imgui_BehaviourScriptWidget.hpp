#pragma once
#include <array>
#include <filesystem>
#include <limits>
#include <optional>
#include <string>
#include <vector>
#include "../Imgui_InspectorWidget.hpp"
#include "modules/asset_management/AssetLoader.hpp"
#include "modules/asset_management/asset_types/LuaScript.hpp"
#include "modules/scene/components/BehaviourScript.hpp"
#include "modules/scripting/ScriptPropertyParser.hpp"

class RuntimeScene;

class Imgui_BehaviourScriptWidget : public Imgui_InspectorWidget {
public:
    Imgui_BehaviourScriptWidget(RuntimeScene& scene, AssetLoader& assetLoader, std::string title) :
        Imgui_InspectorWidget(std::move(title)),
        scene_(scene),
        assetLoader_(assetLoader) {}

    void setCurrentObjectId(EntityHandle objectId) { lastObjectId_.emplace(objectId); }

    void setComponent(const BehaviourScript& c, size_t index) {
        index_ = index;
        component_ = c;
        component_.scriptName.copy(nameBuf_.data(), nameBuf_.size() - 1);
        nameBuf_[component_.scriptName.size()] = '\0';

        // Re-parse property descriptors when script name changes
        if (component_.scriptName != lastParsedScript_) {
            lastParsedScript_ = component_.scriptName;
            descriptors_ = parseDescriptors(component_.scriptName);
            // Seed any missing propertyValues with defaults
            bool changed = false;
            for (const auto& desc: descriptors_) {
                if (!component_.propertyValues.contains(desc.name)) {
                    component_.propertyValues[desc.name] = desc.defaultValue;
                    changed = true;
                }
            }
            if (changed) commitEdit();
        }
    }

private:
    RuntimeScene& scene_;
    AssetLoader& assetLoader_;
    std::optional<EntityHandle> lastObjectId_;
    BehaviourScript component_{};
    std::array<char, 256> nameBuf_{};
    size_t index_ = 0;
    std::string lastParsedScript_;
    std::vector<ScriptPropertyDescriptor> descriptors_;

    // ---- Custom property: vec3 row ----------------------------------------

    class Vec3PropRow : public InspectorProperty {
    public:
        Vec3PropRow(std::string label, std::function<glm::vec3()> get, std::function<void(glm::vec3)> set) :
            label_(std::move(label)),
            get_(std::move(get)),
            set_(std::move(set)) {}

        void draw() override {
            rowNoWidth(label_.c_str(), [&] {
                auto v = get_();
                float w = (ImGui::GetContentRegionAvail().x - 8.0f) / 3.0f;
                ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 0));
                bool changed = false;
                ImGui::SetNextItemWidth(w);
                changed |= drawSpinbox("##x", v.x, 0.01f, 0, 0, "X: %.3f");
                ImGui::SameLine();
                ImGui::SetNextItemWidth(w);
                changed |= drawSpinbox("##y", v.y, 0.01f, 0, 0, "Y: %.3f");
                ImGui::SameLine();
                ImGui::SetNextItemWidth(w);
                changed |= drawSpinbox("##z", v.z, 0.01f, 0, 0, "Z: %.3f");
                ImGui::PopStyleVar();
                if (changed) set_(v);
            });
        }

    private:
        std::string label_;
        std::function<glm::vec3()> get_;
        std::function<void(glm::vec3)> set_;
    };

    // ---- buildProperties --------------------------------------------------

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

        if (descriptors_.empty()) return;

        add<SeparatorProperty>();

        for (const auto& desc: descriptors_) {
            const std::string& key = desc.name;

            if (desc.type == "float") {
                add<DragFloatProperty>(
                        key,
                        [this, key] {
                            auto it = component_.propertyValues.find(key);
                            return it != component_.propertyValues.end() ? it->second.get<float>() : 0.0f;
                        },
                        [this, key](float v) { component_.propertyValues[key] = v; },
                        0.1f,
                        0.0f,
                        0.0f,
                        [this] { commitEdit(); });

            } else if (desc.type == "int") {
                add<SliderIntProperty>(
                        key,
                        [this, key] {
                            auto it = component_.propertyValues.find(key);
                            return it != component_.propertyValues.end() ? it->second.get<int>() : 0;
                        },
                        [this, key](int v) {
                            component_.propertyValues[key] = v;
                            commitEdit();
                        },
                        0,
                        1000);

            } else if (desc.type == "bool") {
                add<CheckboxProperty>(
                        key,
                        [this, key] {
                            auto it = component_.propertyValues.find(key);
                            return it != component_.propertyValues.end() && it->second.get<bool>();
                        },
                        [this, key](bool v) {
                            component_.propertyValues[key] = v;
                            commitEdit();
                        });

            } else if (desc.type == "string") {
                add<LabelProperty>(key, [this, key] {
                    auto it = component_.propertyValues.find(key);
                    return it != component_.propertyValues.end() ? it->second.get<std::string>() : "";
                });

            } else if (desc.type == "vec3") {
                addRaw(std::make_unique<Vec3PropRow>(
                        key,
                        [this, key] {
                            auto it = component_.propertyValues.find(key);
                            if (it == component_.propertyValues.end()) return glm::vec3{};
                            auto& a = it->second;
                            return glm::vec3{a[0].get<float>(), a[1].get<float>(), a[2].get<float>()};
                        },
                        [this, key](glm::vec3 v) {
                            component_.propertyValues[key] = {v.x, v.y, v.z};
                            commitEdit();
                        }));

            } else if (desc.type == "entity") {
                add<EntityProperty>(
                        key,
                        [this, key] {
                            auto it = component_.propertyValues.find(key);
                            if (it == component_.propertyValues.end()) return uint64_t(0);
                            return it->second.get<uint64_t>();
                        },
                        [this, key](uint64_t id) {
                            component_.propertyValues[key] = id;
                            commitEdit();
                        });

            } else if (desc.type == "color") {
                add<ColorEdit4Property>(
                        key,
                        [this, key] {
                            auto it = component_.propertyValues.find(key);
                            if (it == component_.propertyValues.end()) return glm::vec4{1, 1, 1, 1};
                            auto& a = it->second;
                            return glm::vec4{
                                    a[0].get<float>(), a[1].get<float>(), a[2].get<float>(), a[3].get<float>()};
                        },
                        [this, key](glm::vec4 v) {
                            component_.propertyValues[key] = {v.r, v.g, v.b, v.a};
                            commitEdit();
                        });
            }
        }
    }

    void commitEdit() {
        if (!lastObjectId_) return;
        BehaviourScript snapshot = component_;
        size_t index = index_;
        scene_.getObject(*lastObjectId_)
                .mutateComponentAt<BehaviourScript>(
                        index,
                        [snapshot](BehaviourScript& b) { b = snapshot; },
                        UndoHistory::randomGroupId("Edit Script Property"));
    }

    std::vector<ScriptPropertyDescriptor> parseDescriptors(const std::string& scriptName) {
        if (scriptName.empty()) return {};
        auto* script = assetLoader_.get<LuaScript>(scriptName);
        if (!script) return {};
        return ScriptPropertyParser::parse(script->path);
    }
};
