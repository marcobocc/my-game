#pragma once
#include <GLFW/glfw3.h>
#include <array>
#include <cstring>
#include <functional>
#include <imgui.h>
#include <string>
#include <unordered_map>
#include "../../../services/ActionDispatcher.hpp"
#include "../../../services/ClipboardService.hpp"
#include "../../../services/EditorSelection.hpp"
#include "../../../services/common_editing/AssetStore.hpp"
#include "../../../services/common_editing/RuntimeScene.hpp"
#include "../../../services/common_editing/SceneQuickActions.hpp"
#include "../../../styling/EditorPanel.hpp"
#include "../../../styling/ImguiStyling.hpp"
#include "../../input_handling/ShortcutBindingService.hpp"
#include "Imgui_CapsulePopupModal.hpp"
#include "Imgui_HierarchyDropdownMenu.hpp"
#include "Imgui_SpherePopupModal.hpp"
#include "modules/scene/components/Metadata.hpp"

class Imgui_HierarchyPanel : public EditorPanel {
public:
    Imgui_HierarchyPanel(AssetStore& assetStore,
                         EditorSelection& editorSelection,
                         RuntimeScene& scene,
                         ActionDispatcher& actionDispatcher,
                         ShortcutBindingService& shortcutBindingService,
                         SceneQuickActions& sceneQuickActions,
                         ClipboardService& clipboardService) :
        assetStore_(assetStore),
        editorSelection_(editorSelection),
        scene_(scene),
        sceneQuickActions_(sceneQuickActions),
        spherePopupModal_(sceneQuickActions),
        capsulePopupModal_(sceneQuickActions),
        dropdownMenu_(assetStore,
                      scene,
                      sceneQuickActions,
                      actionDispatcher,
                      shortcutBindingService,
                      editorSelection,
                      clipboardService,
                      this,
                      &spherePopupModal_,
                      &capsulePopupModal_) {}

    void draw() { EditorPanel::draw("Objects Hierarchy"); }

    void startRenaming(EntityHandle entity, const std::string& currentName) {
        editingEntity_ = entity;
        editBuffer_.fill('\0');
        std::strncpy(editBuffer_.data(), currentName.c_str(), editBuffer_.size() - 1);
    }

    void drawBody() override {
        contextTargetId.reset();
        spherePopupModal_.draw();
        capsulePopupModal_.draw();
        if (ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows) && ImGui::IsKeyPressed(ImGuiKey_Delete, false)) {
            // routed through ActionDispatcher -> SceneQuickActions
        }
        ImGui::Separator();

        bool cmdDown = ImGui::GetIO().KeySuper || ImGui::GetIO().KeyCtrl;
        bool rightClickedThisFrame = false;
        std::optional<EntityHandle> rightClickedEntity;

        SceneDTO dto = scene_.snapshotScene();

        // Build a lookup: handle -> label and component list
        std::unordered_map<EntityHandle, const GameObjectDTO*> objMap;
        for (const auto& obj: dto.objects)
            objMap[obj.handle] = &obj;

        auto getLabel = [](const GameObjectDTO& obj) -> std::string {
            for (const auto& c: obj.components) {
                if (const auto* meta = std::get_if<Metadata>(&c)) {
                    if (!meta->displayName.empty()) return meta->displayName;
                    break;
                }
            }
            return "Entity " + std::to_string(obj.handle);
        };

        // Collect all leaf (non-group) entity handles in a subtree.
        std::function<void(const HierarchyNode&, std::vector<EntityHandle>&)> collectLeaves =
                [&](const HierarchyNode& node, std::vector<EntityHandle>& out) {
                    if (node.children.empty()) {
                        out.push_back(node.handle);
                    } else {
                        for (const auto& child: node.children)
                            collectLeaves(child, out);
                    }
                };

        // Returns true if every leaf under this node is selected.
        std::function<bool(const HierarchyNode&)> allLeavesSelected = [&](const HierarchyNode& node) -> bool {
            if (node.children.empty()) return editorSelection_.isEntitySelected(node.handle);
            for (const auto& child: node.children)
                if (!allLeavesSelected(child)) return false;
            return true;
        };

        // Forward-declare so drawSiblings can call drawNode recursively.
        std::function<void(const std::vector<HierarchyNode>&)> drawSiblings;

        std::function<void(const HierarchyNode&, bool)> drawNode = [&](const HierarchyNode& node, bool isFirstSibling) {
            EntityHandle e = node.handle;
            const GameObjectDTO* obj = objMap.count(e) ? objMap[e] : nullptr;
            std::string label = obj ? getLabel(*obj) : "Entity " + std::to_string(e);

            bool hasChildren = !node.children.empty();
            // Group nodes show as selected when all their descendants are selected.
            bool selected = hasChildren ? allLeavesSelected(node) : editorSelection_.isEntitySelected(e);
            bool isContextTarget = contextTargetId && *contextTargetId == e;

            ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
            if (!hasChildren) flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
            if (selected || isContextTarget) flags |= ImGuiTreeNodeFlags_Selected;

            bool nodeOpen = false;
            if (editingEntity_ == e) {
                // Draw tree node label area but replace text with input
                ImGui::TreeNodeEx(("##node" + std::to_string(e)).c_str(),
                                  flags | ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen);
                ImGui::SameLine();
                if (!editingStartedThisFrame_) {
                    ImGui::SetKeyboardFocusHere();
                    editingStartedThisFrame_ = true;
                }
                if (ImGui::InputText(("##rename_" + std::to_string(e)).c_str(),
                                     editBuffer_.data(),
                                     editBuffer_.size(),
                                     ImGuiInputTextFlags_EnterReturnsTrue)) {
                    std::string newName(editBuffer_.data());
                    if (!newName.empty()) sceneQuickActions_.renameObject(e, newName);
                    editingEntity_.reset();
                    editBuffer_.fill('\0');
                    editingStartedThisFrame_ = false;
                }
                bool itemActive = ImGui::IsItemActive();
                if (!itemActive && editingStartedThisFrame_) {
                    // wait for focus
                } else if (!itemActive) {
                    editingEntity_.reset();
                    editBuffer_.fill('\0');
                    editingStartedThisFrame_ = false;
                }
                if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
                    editingEntity_.reset();
                    editBuffer_.fill('\0');
                    editingStartedThisFrame_ = false;
                }
            } else {
                nodeOpen = ImGui::TreeNodeEx(("##node" + std::to_string(e)).c_str(), flags, "%s", label.c_str());
            }

            bool hovered = ImGui::IsItemHovered() || isContextTarget;
            if (hovered && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                if (hasChildren) {
                    // Clicking a group selects/deselects all its leaf descendants.
                    std::vector<EntityHandle> leaves;
                    collectLeaves(node, leaves);
                    if (cmdDown && selected) {
                        for (EntityHandle leaf: leaves)
                            editorSelection_.removeFromSelection(leaf);
                    } else {
                        if (!cmdDown) editorSelection_.clearSelection();
                        for (EntityHandle leaf: leaves)
                            editorSelection_.addToSelection(leaf, true);
                    }
                } else {
                    if (cmdDown && editorSelection_.isEntitySelected(e))
                        editorSelection_.removeFromSelection(e);
                    else
                        editorSelection_.addToSelection(e, cmdDown);
                }
            }
            if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
                rightClickedThisFrame = true;
                rightClickedEntity.emplace(e);
            }

            // Drag source
            if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
                ImGui::SetDragDropPayload("HIERARCHY_ENTITY", &e, sizeof(EntityHandle));
                ImGui::TextUnformatted(label.c_str());
                ImGui::EndDragDropSource();
            }

            // Drop target: bottom half = insert after (line), top half = reparent onto (rect).
            // Always suppress ImGui's default rect and draw feedback manually so the two
            // zones never overlap visually.
            if (ImGui::BeginDragDropTarget()) {
                ImVec2 rMin = ImGui::GetItemRectMin();
                ImVec2 rMax = ImGui::GetItemRectMax();
                bool botZone = ImGui::GetMousePos().y >= rMin.y + (rMax.y - rMin.y) / 2.0f;
                ImDrawList* dl = ImGui::GetWindowDrawList();
                ImU32 col = ImGui::GetColorU32(ImGuiCol_DragDropTarget);

                if (botZone) {
                    dl->AddLine({rMin.x, rMax.y}, {rMax.x, rMax.y}, col, 2.0f);
                    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(
                                "HIERARCHY_ENTITY", ImGuiDragDropFlags_AcceptNoDrawDefaultRect)) {
                        EntityHandle dragged = *static_cast<const EntityHandle*>(payload->Data);
                        if (dragged != e) sceneQuickActions_.reorder(dragged, e, false);
                    }
                } else if (!isFirstSibling || ImGui::GetDragDropPayload() == nullptr) {
                    // Suppress the reparent rect for the first sibling when a drag is active:
                    // the invisible button above it already owns that zone.
                    dl->AddRect(rMin, rMax, col);
                    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(
                                "HIERARCHY_ENTITY", ImGuiDragDropFlags_AcceptNoDrawDefaultRect)) {
                        EntityHandle dragged = *static_cast<const EntityHandle*>(payload->Data);
                        if (dragged != e) sceneQuickActions_.reparent(dragged, e);
                    }
                } else {
                    ImGui::AcceptDragDropPayload("HIERARCHY_ENTITY", ImGuiDragDropFlags_AcceptNoDrawDefaultRect);
                }
                ImGui::EndDragDropTarget();
            }

            if (hasChildren && nodeOpen) {
                drawSiblings(node.children);
                ImGui::TreePop();
            }
        };

        // Draws a sibling list. Before the first sibling, renders a thin invisible drop zone
        // so dragging above the first item shows a line and triggers "insert before" it.
        drawSiblings = [&](const std::vector<HierarchyNode>& siblings) {
            for (size_t i = 0; i < siblings.size(); ++i) {
                bool isFirst = (i == 0);
                if (isFirst && ImGui::GetDragDropPayload() != nullptr) {
                    ImVec2 cursor = ImGui::GetCursorScreenPos();
                    ImGui::InvisibleButton("##drop_before_first", {ImGui::GetContentRegionAvail().x, 4.0f});
                    if (ImGui::BeginDragDropTarget()) {
                        ImVec2 rMin = ImGui::GetItemRectMin();
                        ImVec2 rMax = ImGui::GetItemRectMax();
                        ImGui::GetWindowDrawList()->AddLine(
                                {rMin.x, rMin.y}, {rMax.x, rMin.y}, ImGui::GetColorU32(ImGuiCol_DragDropTarget), 2.0f);
                        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(
                                    "HIERARCHY_ENTITY", ImGuiDragDropFlags_AcceptNoDrawDefaultRect)) {
                            EntityHandle dragged = *static_cast<const EntityHandle*>(payload->Data);
                            EntityHandle first = siblings[0].handle;
                            if (dragged != first) sceneQuickActions_.reorder(dragged, first, true);
                        }
                        ImGui::EndDragDropTarget();
                    }
                    // Collapse the invisible button when not a drop target so layout is unaffected.
                    if (!ImGui::IsItemActive()) ImGui::SetCursorScreenPos(cursor);
                }
                drawNode(siblings[i], isFirst);
            }
        };

        drawSiblings(dto.hierarchy);


        if (!rightClickedThisFrame && ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
            rightClickedThisFrame = true;
            rightClickedEntity.reset();
        }

        if (rightClickedThisFrame) {
            contextTargetId = rightClickedEntity;
            if (rightClickedEntity) {
                std::string label = "Entity " + std::to_string(*rightClickedEntity);
                for (const auto& obj: dto.objects) {
                    if (obj.handle == *rightClickedEntity) {
                        for (const auto& c: obj.components) {
                            if (const auto* meta = std::get_if<Metadata>(&c)) {
                                if (!meta->displayName.empty()) label = meta->displayName;
                                break;
                            }
                        }
                        break;
                    }
                }
                dropdownMenu_.setContextEntity(*rightClickedEntity, label);
            }
            ImGui::OpenPopup("HierarchyContextMenu");
        }

        dropdownMenu_.draw("HierarchyContextMenu");

        if (auto renameEntity = dropdownMenu_.getAndClearRenameRequest()) {
            startRenaming(*renameEntity, dropdownMenu_.getRenameRequestName());
        }
    }

private:
    static constexpr size_t NAME_BUFFER_SIZE = 256;

    AssetStore& assetStore_;
    EditorSelection& editorSelection_;
    RuntimeScene& scene_;
    SceneQuickActions& sceneQuickActions_;
    Imgui_SpherePopupModal spherePopupModal_;
    Imgui_CapsulePopupModal capsulePopupModal_;
    Imgui_HierarchyDropdownMenu dropdownMenu_;

    std::optional<EntityHandle> contextTargetId;
    std::optional<EntityHandle> editingEntity_;
    std::array<char, NAME_BUFFER_SIZE> editBuffer_{};
    bool editingStartedThisFrame_ = false;
};
