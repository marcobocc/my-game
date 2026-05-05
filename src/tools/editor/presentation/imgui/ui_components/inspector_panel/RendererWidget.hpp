#pragma once
#include <glm/glm.hpp>
#include <imgui.h>
#include <optional>
#include <string>
#include <unordered_set>
#include "data/assets/Material.hpp"
#include "data/components/Renderer.hpp"
#include "systems/assets/AssetManager.hpp"

class SceneMutations;
class EditorGizmos;

class RendererWidget {
public:
    RendererWidget(AssetManager& assetManager, SceneMutations& sceneMutations, EditorGizmos& debugViz) :
        assetManager_(assetManager),
        sceneMutations_(sceneMutations),
        debugViz_(debugViz) {}

    void draw(Renderer& r, const std::string& objectId) {
        int rows = 5;
        rows += 2; // AABB and bounding sphere toggles

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
            if (ImGui::ColorEdit4("Base color", col)) {
                r.baseColorOverride = glm::vec4(col[0], col[1], col[2], col[3]);
            }
            if (ImGui::IsItemActivated()) {
                sceneMutations_.beginEditByType(objectId, "Renderer");
            }
            if (ImGui::IsItemDeactivatedAfterEdit()) {
                sceneMutations_.endEditByType(objectId, "Renderer");
            }
        }

        ImGui::Spacing();
        auto aabbEnabled = debugViz_.getObjectsWithAABBEnabled();
        bool showAABB = aabbEnabled.count(objectId) > 0;
        if (ImGui::Checkbox("Show Bounding Box", &showAABB)) {
            debugViz_.toggleAABB(objectId);
        }

        ImGui::Spacing();
        auto sphereEnabled = debugViz_.getObjectsWithBoundingSpheresEnabled();
        bool showSphere = sphereEnabled.count(objectId) > 0;
        if (ImGui::Checkbox("Show Bounding Sphere", &showSphere)) {
            debugViz_.toggleBoundingSphere(objectId);
        }

        ImGui::EndChild();
    }

private:
    AssetManager& assetManager_;
    SceneMutations& sceneMutations_;
    EditorGizmos& debugViz_;

    static float childHeight(int rows) {
        return ImGui::GetFrameHeightWithSpacing() * static_cast<float>(rows) + ImGui::GetStyle().WindowPadding.y * 2.0f;
    }
};
