#pragma once
#include <functional>
#include <imgui.h>
#include <optional>
#include "../../../../../../engine/modules/ui/ImguiWidget.hpp"
#include "../../ImguiStyling.hpp"
#include "../hierarchy/HierarchyDropdownMenu.hpp"
#include "../hierarchy/SpherePopupModal.hpp"
#include "modules/scene/components/Renderer.hpp"

class SceneMutations;
class EditorAssetRepository;
class EditorSelection;
class GameWindow;
class EditorCamera;
class PickingSystem;
class ObjectBuilder;

class EditorSceneViewport : public ImguiWidget {
public:
    static constexpr float DRAG_THRESHOLD = 25.0f;

    EditorSceneViewport(EditorAssetRepository& assetRepository,
                        SceneMutations& sceneMutations,
                        EditorSelection& editorSelection,
                        EntityManager& entityManager,
                        PickingSystem& pickingSystem,
                        GameWindow& window,
                        EditorCamera& editorCamera,
                        ObjectBuilder& objectBuilder,
                        ActionDispatcher& actionDispatcher,
                        ShortcutBindingService& shortcutBindingService) :
        assetRepository_(assetRepository),
        sceneMutations_(sceneMutations),
        editorSelection_(editorSelection),
        entityManager_(entityManager),
        pickingSystem_(pickingSystem),
        window_(window),
        editorCamera_(editorCamera),
        objectBuilder_(objectBuilder),
        spherePopupModal_(sceneMutations_, objectBuilder_),
        dropdownMenu_(assetRepository,
                      sceneMutations,
                      objectBuilder,
                      actionDispatcher,
                      shortcutBindingService,
                      &spherePopupModal_) {}

    void draw() override {
        spherePopupModal_.draw();

        auto sv = window_.getSceneViewport();
        ImVec2 mousePos = ImGui::GetMousePos();
        bool isInViewport = mousePos.x >= sv.x && mousePos.x < sv.x + sv.width && mousePos.y >= sv.y &&
                            mousePos.y < sv.y + sv.height;

        // Handle drag-drop material attachment
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
                            if (auto* renderer = entityManager_.getComponent<Renderer>(hitId)) {
                                sceneMutations_.beginEdit(hitId);
                                renderer->materialName = materialName;
                                sceneMutations_.commitEdit(hitId);
                            }
                        }
                    }
                }
            }
        }

        // Track right-click without drag
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
            rightClickStartX_ = mousePos.x;
            rightClickStartY_ = mousePos.y;
            hasMovedSinceRightClick_ = false;
        }

        // Check if mouse moved since right-click
        if (ImGui::IsMouseDown(ImGuiMouseButton_Right)) {
            ImVec2 mousePos = ImGui::GetMousePos();
            float dx = mousePos.x - rightClickStartX_;
            float dy = mousePos.y - rightClickStartY_;
            if (dx * dx + dy * dy > DRAG_THRESHOLD) {
                hasMovedSinceRightClick_ = true;
            }
        }

        // Open popup only if right-click released without significant movement
        if (ImGui::IsMouseReleased(ImGuiMouseButton_Right) && !hasMovedSinceRightClick_ && isInViewport) {
            // Use picking system to see what was clicked
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

            // Show delete only if we hit an object AND it's currently selected
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

                if (hasHit && editorSelection_.isEntitySelected(hitId)) {
                    contextTargetId_ = hitId;
                }
            }
            ImGui::OpenPopup("ViewportContextMenu");
        }

        ImguiStyling::withPopup("ViewportContextMenu", [&] { dropdownMenu_.draw(contextTargetId_); });
    }

private:
    EditorAssetRepository& assetRepository_;
    SceneMutations& sceneMutations_;
    EditorSelection& editorSelection_;
    EntityManager& entityManager_;
    PickingSystem& pickingSystem_;
    GameWindow& window_;
    EditorCamera& editorCamera_;
    ObjectBuilder& objectBuilder_;
    SpherePopupModal spherePopupModal_;
    HierarchyDropdownMenu dropdownMenu_;
    float rightClickStartX_ = 0.0f;
    float rightClickStartY_ = 0.0f;
    bool hasMovedSinceRightClick_ = false;
    std::optional<EntityHandle> contextTargetId_;
};
