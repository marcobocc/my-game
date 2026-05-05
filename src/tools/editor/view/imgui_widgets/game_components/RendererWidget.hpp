#pragma once
#include <functional>
#include <glm/glm.hpp>
#include <imgui.h>
#include <optional>
#include <string>
#include <unordered_set>
#include "../../../controller/SceneMutationsController.hpp"
#include "data/assets/Material.hpp"
#include "data/components/Renderer.hpp"
#include "systems/assets/AssetManager.hpp"

class RendererWidget {
public:
    using AABBToggleCallback = std::function<void(const std::string&, bool)>;
    using BoundingSphereToggleCallback = std::function<void(const std::string&, bool)>;

    RendererWidget(AssetManager& assetManager,
                   SceneMutationsController& mutations,
                   AABBToggleCallback aabbToggle = nullptr,
                   BoundingSphereToggleCallback sphereToggle = nullptr) :
        assetManager_(assetManager),
        mutations_(mutations),
        aabbToggle_(aabbToggle),
        sphereToggle_(sphereToggle) {}

    void draw(Renderer& r, const std::string& objectId) const {
        int rows = 5;
        if (aabbToggle_) rows++;
        if (sphereToggle_) rows++;
        if (!ImGui::BeginChild("Renderer", {0, childHeight(rows)}, true)) {
            ImGui::EndChild();
            return;
        }
        ImGui::TextColored({0.8f, 0.7f, 0.2f, 1.0f}, "Renderer");
        ImGui::Spacing();
        ImGui::Text("Mesh: %s", r.meshName.c_str());
        ImGui::Text("Material: %s", r.materialName.c_str());

        if (const Material* mat = assetManager_.get<Material>(r.materialName)) {
            if (!mat->getTextureName().empty())
                ImGui::Text("Texture: %s", mat->getTextureName().c_str());
            else
                ImGui::TextDisabled("Texture: none");
            glm::vec4 color = r.baseColorOverride.value_or(mat->getBaseColor());
            float col[4] = {color.r, color.g, color.b, color.a};
            auto colorBefore = r.baseColorOverride;
            if (ImGui::ColorEdit4("Base color", col)) r.baseColorOverride = glm::vec4(col[0], col[1], col[2], col[3]);
            if (ImGui::IsItemActivated()) colorBeforeEdit_ = colorBefore;
            if (ImGui::IsItemDeactivatedAfterEdit()) {
                auto oldVal = colorBeforeEdit_;
                auto newVal = r.baseColorOverride;
                auto* ptr = &r.baseColorOverride;
                mutations_.undoHistory().push([ptr, oldVal] { *ptr = oldVal; }, [ptr, newVal] { *ptr = newVal; });
            }
        }

        if (aabbToggle_) {
            ImGui::Spacing();
            static std::unordered_set<std::string> aabbToggled;
            bool showAABB = aabbToggled.count(objectId) > 0;
            if (ImGui::Checkbox("Show Bounding Box", &showAABB)) {
                if (showAABB) {
                    aabbToggled.insert(objectId);
                    aabbToggle_(objectId, true);
                } else {
                    aabbToggled.erase(objectId);
                    aabbToggle_(objectId, false);
                }
            }
        }

        if (sphereToggle_) {
            ImGui::Spacing();
            static std::unordered_set<std::string> sphereToggled;
            bool showSphere = sphereToggled.count(objectId) > 0;
            if (ImGui::Checkbox("Show Bounding Sphere", &showSphere)) {
                if (showSphere) {
                    sphereToggled.insert(objectId);
                    sphereToggle_(objectId, true);
                } else {
                    sphereToggled.erase(objectId);
                    sphereToggle_(objectId, false);
                }
            }
        }

        ImGui::EndChild();
    }

private:
    AssetManager& assetManager_;
    SceneMutationsController& mutations_;
    AABBToggleCallback aabbToggle_;
    BoundingSphereToggleCallback sphereToggle_;
    mutable std::optional<glm::vec4> colorBeforeEdit_;

    static float childHeight(int rows) {
        return ImGui::GetFrameHeightWithSpacing() * static_cast<float>(rows) + ImGui::GetStyle().WindowPadding.y * 2.0f;
    }
};
