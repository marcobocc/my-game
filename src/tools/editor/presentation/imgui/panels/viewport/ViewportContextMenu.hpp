#pragma once
#include <functional>
#include <imgui.h>
#include <optional>
#include "../../../../../../engine/modules/ui/ImguiWidget.hpp"
#include "../../ImguiStyling.hpp"
#include "../hierarchy/HierarchyDropdownMenu.hpp"
#include "../hierarchy/SpherePopupModal.hpp"

class SceneMutations;
class AssetManager;
class ObjectSelection;
class GameWindow;
class EditorCamera;
class PickingSystem;

struct SceneObjectHit;
struct GizmoHit;

using EntityHandle = uint64_t;

class ViewportContextMenu : public ImguiWidget {
public:
    static constexpr float DRAG_THRESHOLD = 25.0f; // 5 pixels squared

    ViewportContextMenu(AssetManager& assetManager,
                        SceneMutations& sceneMutations,
                        ObjectSelection& objectSelection,
                        EntityManager& entityManager,
                        PickingSystem& pickingSystem,
                        GameWindow& window,
                        EditorCamera& editorCamera,
                        std::function<void(uint32_t)> onSphereCreated = nullptr) :
        assetManager_(assetManager),
        sceneMutations_(sceneMutations),
        objectSelection_(objectSelection),
        entityManager_(entityManager),
        pickingSystem_(pickingSystem),
        window_(window),
        editorCamera_(editorCamera),
        spherePopupModal_(onSphereCreated),
        dropdownMenu_(assetManager, sceneMutations, &spherePopupModal_) {}

    void draw() override {
        spherePopupModal_.draw();

        // Track right-click without drag
        auto sv = window_.getSceneViewport();
        ImVec2 mousePos = ImGui::GetMousePos();
        bool isInViewport = mousePos.x >= sv.x && mousePos.x < sv.x + sv.width && mousePos.y >= sv.y &&
                            mousePos.y < sv.y + sv.height;

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
            auto sv = window_.getSceneViewport();
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

                if (hasHit) {
                    auto selectedId = objectSelection_.getSelectedEntityId();
                    if (selectedId && *selectedId == hitId) {
                        contextTargetId_ = hitId;
                    }
                }
            }
            ImGui::OpenPopup("ViewportContextMenu");
        }

        ImguiStyling::withPopup("ViewportContextMenu", [&] { dropdownMenu_.draw(contextTargetId_); });
    }

private:
    AssetManager& assetManager_;
    SceneMutations& sceneMutations_;
    ObjectSelection& objectSelection_;
    EntityManager& entityManager_;
    PickingSystem& pickingSystem_;
    GameWindow& window_;
    EditorCamera& editorCamera_;
    SpherePopupModal spherePopupModal_;
    HierarchyDropdownMenu dropdownMenu_;
    float rightClickStartX_ = 0.0f;
    float rightClickStartY_ = 0.0f;
    bool hasMovedSinceRightClick_ = false;
    std::optional<EntityHandle> contextTargetId_;
};
