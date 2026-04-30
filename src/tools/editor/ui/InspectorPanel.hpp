#pragma once
#include <glm/gtc/quaternion.hpp>
#include <imgui.h>
#include <optional>
#include <string>
#include "../controllers/EditorGizmosController.hpp"
#include "../controllers/SceneMutationsController.hpp"
#include "GameEngine.hpp"
#include "data/assets/Material.hpp"
#include "data/components/BoxCollider.hpp"
#include "data/components/Renderer.hpp"
#include "data/components/Transform.hpp"
#include "systems/ui/ImguiWidget.hpp"

class InspectorPanel : public ImguiWidget {
public:
    InspectorPanel(const std::optional<std::string>* selectedObjectId,
                   GameEngine& engine,
                   EditorGizmosController& gizmos,
                   SceneMutationsController& mutations) :
        selectedObjectId_(selectedObjectId),
        engine_(engine),
        gizmos_(gizmos),
        mutations_(mutations) {}

    void draw() const override {
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        float menuBarHeight = ImGui::GetFrameHeight();
        float width = viewport->Size.x * PANEL_WIDTH_RATIO;
        ImGui::SetNextWindowPos({viewport->Pos.x + viewport->Size.x - width, viewport->Pos.y + menuBarHeight});
        ImGui::SetNextWindowSize({width, viewport->Size.y - menuBarHeight});
        ImGui::SetNextWindowBgAlpha(0.95f);

        constexpr ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
                                           ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                                           ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
        ImGui::Begin("Inspector", nullptr, flags);

        ImGui::Spacing();
        ImGui::TextColored({0.8f, 0.7f, 0.2f, 1.0f}, "Inspector");
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (!selectedObjectId_ || !selectedObjectId_->has_value()) {
            ImGui::TextDisabled("No object selected");
        } else {
            drawObject(engine_.getObject(**selectedObjectId_));
        }

        ImGui::End();
    }

    static constexpr float PANEL_WIDTH_RATIO = 0.25f;

private:
    const std::optional<std::string>* selectedObjectId_;
    GameEngine& engine_;
    EditorGizmosController& gizmos_;
    SceneMutationsController& mutations_;
    mutable std::optional<glm::vec4> colorBeforeEdit_;

    static float childHeight(int rows) {
        return ImGui::GetFrameHeightWithSpacing() * static_cast<float>(rows) + ImGui::GetStyle().WindowPadding.y * 2.0f;
    }

    template<typename T>
    void trackDrag(T& target) const {
        if (ImGui::IsItemActivated()) mutations_.beginEdit(target);
        if (ImGui::IsItemDeactivatedAfterEdit()) mutations_.commitEdit(target);
    }

    void drawObject(GameObject& obj) const {
        if (obj.has<Transform>()) drawTransform(obj.get<Transform>());
        ImGui::Spacing();
        if (obj.has<Renderer>()) drawRenderer(obj.get<Renderer>(), obj.getName());
        ImGui::Spacing();
        if (obj.has<BoxCollider>()) drawBoxCollider(obj.get<BoxCollider>());
    }

    void drawTransform(Transform& t) const {
        if (!ImGui::BeginChild("Transform", {0, childHeight(4)}, true)) {
            ImGui::EndChild();
            return;
        }
        ImGui::TextColored({0.8f, 0.7f, 0.2f, 1.0f}, "Transform");
        ImGui::Spacing();
        ImGui::DragFloat3("Position", &t.position.x, 0.01f);
        trackDrag(t.position);
        glm::vec3 euler = glm::degrees(glm::eulerAngles(t.rotation));
        if (ImGui::DragFloat3("Rotation", &euler.x, 0.5f)) t.rotation = glm::quat(glm::radians(euler));
        trackDrag(t.rotation);
        ImGui::DragFloat3("Scale", &t.scale.x, 0.01f);
        trackDrag(t.scale);
        ImGui::EndChild();
    }

    void drawRenderer(Renderer& r, const std::string& objectId) const {
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

    void drawBoxCollider(BoxCollider& b) const {
        if (!ImGui::BeginChild("BoxCollider", {0, childHeight(3)}, true)) {
            ImGui::EndChild();
            return;
        }
        ImGui::TextColored({0.8f, 0.7f, 0.2f, 1.0f}, "Box Collider");
        ImGui::Spacing();
        ImGui::DragFloat3("Center", &b.center.x, 0.01f);
        trackDrag(b.center);
        ImGui::DragFloat3("Half Extents", &b.halfExtents.x, 0.01f);
        trackDrag(b.halfExtents);
        ImGui::EndChild();
    }
};
