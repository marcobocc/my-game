#pragma once
#include <imgui.h>
#include <optional>
#include <string>
#include "../../controllers/EditorGizmosController.hpp"
#include "../../controllers/SceneMutationsController.hpp"
#include "GameEngine.hpp"
#include "data/assets/Material.hpp"
#include "data/components/Renderer.hpp"

class RendererWidget {
public:
    RendererWidget(GameEngine& engine, EditorGizmosController& gizmos, SceneMutationsController& mutations) :
        engine_(engine),
        gizmos_(gizmos),
        mutations_(mutations) {}

    void draw(Renderer& r, const std::string& objectId) const {
        if (!ImGui::BeginChild("Renderer", {0, childHeight(6)}, true)) {
            ImGui::EndChild();
            return;
        }
        ImGui::TextColored({0.8f, 0.7f, 0.2f, 1.0f}, "Renderer");
        ImGui::Spacing();
        ImGui::Text("Mesh: %s", r.meshName.c_str());
        ImGui::Text("Material: %s", r.materialName.c_str());
        if (const Material* mat = engine_.getAsset<Material>(r.materialName)) {
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
        bool showAABB = gizmos_.isAABBEnabled(objectId);
        if (ImGui::Checkbox("Show Mesh Bounding Box", &showAABB)) {
            if (showAABB)
                gizmos_.enableAABB(objectId);
            else
                gizmos_.disableAABB(objectId);
        }
        ImGui::EndChild();
    }

private:
    GameEngine& engine_;
    EditorGizmosController& gizmos_;
    SceneMutationsController& mutations_;
    mutable std::optional<glm::vec4> colorBeforeEdit_;

    static float childHeight(int rows) {
        return ImGui::GetFrameHeightWithSpacing() * static_cast<float>(rows) + ImGui::GetStyle().WindowPadding.y * 2.0f;
    }
};
