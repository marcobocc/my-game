#pragma once
#include <imgui.h>
#include "../../../services/ClipboardService.hpp"
#include "../../scene_hierarchy/imgui/Imgui_HierarchyDropdownMenu.hpp"
#include "../editor_camera/EditorCamera.hpp"
#include "../picking/PickingSystem.hpp"
#include "modules/scene/components/Renderer.hpp"

class RuntimeScene;
class AssetStore;
class EditorSelection;
class GameWindow;

class Imgui_EditorSceneViewport {
public:
    static constexpr float DRAG_THRESHOLD = 25.0f;

    Imgui_EditorSceneViewport(AssetStore& assetStore,
                              RuntimeScene& scene,
                              EditorSelection& editorSelection,
                              World& entityManager,
                              PickingSystem& pickingSystem,
                              GameWindow& window,
                              EditorCamera& editorCamera,
                              ActionDispatcher& actionDispatcher,
                              ShortcutBindingService& shortcutBindingService,
                              ClipboardService& clipboardService,
                              SceneQuickActions& sceneQuickActions) :
        assetStore_(assetStore),
        scene_(scene),
        editorSelection_(editorSelection),
        entityManager_(entityManager),
        pickingSystem_(pickingSystem),
        window_(window),
        editorCamera_(editorCamera),
        spherePopupModal_(sceneQuickActions),
        dropdownMenu_(assetStore,
                      sceneQuickActions,
                      actionDispatcher,
                      shortcutBindingService,
                      editorSelection,
                      clipboardService,
                      &spherePopupModal_) {}

    void draw() {
        spherePopupModal_.draw();

        auto sv = window_.getSceneViewport();
        ImVec2 mousePos = ImGui::GetMousePos();
        bool isInViewport = mousePos.x >= sv.x && mousePos.x < sv.x + sv.width && mousePos.y >= sv.y &&
                            mousePos.y < sv.y + sv.height;

        if (isInViewport && ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
            if (const ImGuiPayload* payload = ImGui::GetDragDropPayload()) {
                if (strcmp(payload->DataType, "MATERIAL_ASSET") == 0) {
                    auto materialName = static_cast<const char*>(payload->Data);
                    auto result = pickingSystem_.pick(static_cast<uint32_t>(mousePos.x),
                                                      static_cast<uint32_t>(mousePos.y),
                                                      static_cast<uint32_t>(sv.x),
                                                      static_cast<uint32_t>(sv.y),
                                                      static_cast<uint32_t>(sv.width),
                                                      static_cast<uint32_t>(sv.height),
                                                      editorCamera_.getCamera(),
                                                      editorCamera_.getCameraTransform(),
                                                      entityManager_);

                    if (result) {
                        EntityHandle hitId = {};
                        bool hasHit = false;

                        if (auto* sceneHit = std::get_if<SceneObjectHit>(&*result)) {
                            hitId = sceneHit->objectId;
                            hasHit = true;
                        } else if (auto* gizmoHit = std::get_if<GizmoHit>(&*result)) {
                            hitId = gizmoHit->objectId;
                            hasHit = true;
                        }

                        if (hasHit) {
                            std::string mat = materialName;
                            scene_.getObject(hitId).mutateComponent<Renderer>(
                                    [mat](Renderer& r) { r.materialName = mat; },
                                    UndoHistory::randomGroupId("Apply Material"));
                        }
                    }
                }
            }
        }

        if (ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
            rightClickStartX_ = mousePos.x;
            rightClickStartY_ = mousePos.y;
            hasMovedSinceRightClick_ = false;
        }

        if (ImGui::IsMouseDown(ImGuiMouseButton_Right)) {
            ImVec2 currentPos = ImGui::GetMousePos();
            float dx = currentPos.x - rightClickStartX_;
            float dy = currentPos.y - rightClickStartY_;
            if (dx * dx + dy * dy > DRAG_THRESHOLD) hasMovedSinceRightClick_ = true;
        }

        if (ImGui::IsMouseReleased(ImGuiMouseButton_Right) && !hasMovedSinceRightClick_ && isInViewport) {
            auto result = pickingSystem_.pick(static_cast<uint32_t>(rightClickStartX_),
                                              static_cast<uint32_t>(rightClickStartY_),
                                              static_cast<uint32_t>(sv.x),
                                              static_cast<uint32_t>(sv.y),
                                              static_cast<uint32_t>(sv.width),
                                              static_cast<uint32_t>(sv.height),
                                              editorCamera_.getCamera(),
                                              editorCamera_.getCameraTransform(),
                                              entityManager_);

            contextTargetId_.reset();

            if (result) {
                EntityHandle hitId = {};
                bool hasHit = false;

                if (auto* sceneHit = std::get_if<SceneObjectHit>(&*result)) {
                    hitId = sceneHit->objectId;
                    hasHit = true;
                } else if (auto* gizmoHit = std::get_if<GizmoHit>(&*result)) {
                    hitId = gizmoHit->objectId;
                    hasHit = true;
                }

                if (hasHit && editorSelection_.isEntitySelected(hitId)) contextTargetId_ = hitId;
            }
            ImGui::OpenPopup("ViewportContextMenu");
        }

        dropdownMenu_.draw("ViewportContextMenu");
    }

private:
    AssetStore& assetStore_;
    RuntimeScene& scene_;
    EditorSelection& editorSelection_;
    World& entityManager_;
    PickingSystem& pickingSystem_;
    GameWindow& window_;
    EditorCamera& editorCamera_;
    Imgui_SpherePopupModal spherePopupModal_;
    Imgui_HierarchyDropdownMenu dropdownMenu_;
    float rightClickStartX_ = 0.0f;
    float rightClickStartY_ = 0.0f;
    bool hasMovedSinceRightClick_ = false;
    std::optional<EntityHandle> contextTargetId_;
};
