#include "SceneQuickActions.hpp"
#include <functional>
#include <optional>
#include <unordered_set>
#include "transport/SceneDTO.hpp"

void SceneQuickActions::deleteSelection() {
    auto ids = editorSelection_.getSelectedEntityIds();
    if (ids.empty()) return;
    auto groupId = UndoHistory::randomGroupId("Delete Selection");

    // Collect all entities to destroy: for each selected entity, also collect
    // all descendants of any group nodes that contain it (and the group itself).
    // We walk the full hierarchy and destroy any node whose entire subtree is selected.
    auto hierarchy = scene_.getHierarchy();
    std::unordered_set<EntityHandle> toDestroy(ids.begin(), ids.end());

    // Recursively mark a group for destruction if all its leaves are selected.
    std::function<bool(const HierarchyNode&)> markIfFullySelected = [&](const HierarchyNode& node) -> bool {
        if (node.children.empty()) return toDestroy.count(node.handle) > 0;
        bool allChildren = true;
        for (const auto& child: node.children)
            allChildren &= markIfFullySelected(child);
        if (allChildren) toDestroy.insert(node.handle);
        return allChildren;
    };
    for (const auto& node: hierarchy)
        markIfFullySelected(node);

    for (EntityHandle e: toDestroy)
        scene_.destroyObject(e, groupId);
    editorSelection_.clearSelection();
}

void SceneQuickActions::copySelection() {
    auto ids = editorSelection_.getSelectedEntityIds();
    if (ids.empty()) return;
    nlohmann::json payload = nlohmann::json::array();
    for (EntityHandle e: ids)
        payload.push_back(scene_.snapshotObject(e).serialize());
    editorClipboard_.copy(payload, ClipboardPayloadType::Entity);
}

void SceneQuickActions::cutSelected() {
    copySelection();
    deleteSelection();
}

void SceneQuickActions::pasteSelection() {
    if (!editorClipboard_.hasEntity()) return;
    const auto& payload = *editorClipboard_.get();
    auto groupId = UndoHistory::randomGroupId("Paste Selection");
    for (const auto& objJson: payload) {
        GameObjectDTO dto = GameObjectDTO::deserialize(objJson);
        dto.handle = scene_.createObject(groupId);
        scene_.restoreObject(dto, groupId);
    }
}

void SceneQuickActions::duplicateSelection() {
    auto ids = editorSelection_.getSelectedEntityIds();
    if (ids.empty()) return;
    auto groupId = UndoHistory::randomGroupId("Duplicaye Selection");
    for (EntityHandle e: ids) {
        GameObjectDTO dto = scene_.snapshotObject(e);
        dto.handle = scene_.createObject(groupId);
        scene_.restoreObject(dto, groupId);
    }
}

void SceneQuickActions::createEmptyObject() {
    auto groupId = UndoHistory::randomGroupId("Create Empty Object");
    EntityHandle handle = scene_.createObject(groupId);
    RuntimeGameObject obj = scene_.getObject(handle);
    obj.addComponent<Metadata>(Metadata{"GameObject"}, groupId);
    obj.addComponent(Transform{glm::vec3(0.0f), glm::quat(1.0f, 0.0f, 0.0f, 0.0f), glm::vec3(1.0f)}, groupId);
}

void SceneQuickActions::createLight() {
    auto groupId = UndoHistory::randomGroupId("Create Light");
    Transform t{glm::vec3(0.0f),
                glm::angleAxis(glm::radians(135.0f), glm::normalize(glm::vec3(1.0f, 1.0f, 0.0f))),
                glm::vec3(1.0f)};
    Light l{LightType::DIRECTIONAL, 1.0f};

    EntityHandle handle = scene_.createObject(groupId);
    RuntimeGameObject obj = scene_.getObject(handle);
    obj.addComponent(Metadata{"Light"}, groupId);
    obj.addComponent(t, groupId);
    obj.addComponent(l, groupId);
}

void SceneQuickActions::createCube() {
    auto groupId = UndoHistory::randomGroupId("Create Cube");
    createPrimitiveGeometry("Cube.mesh", "Cube", groupId);
}

void SceneQuickActions::createPlane() {
    auto groupId = UndoHistory::randomGroupId("Create Plane");
    createPrimitiveGeometry("Plane.mesh", "Plane", groupId);
}

void SceneQuickActions::createSphere(uint32_t resolution) {
    auto groupId = UndoHistory::randomGroupId("Create Sphere");
    std::string meshName = "Sphere_" + std::to_string(resolution) + ".mesh";
    createPrimitiveGeometry(meshName, "Sphere", groupId);
}

void SceneQuickActions::addModel(const std::string& modelName) {
    auto groupId = UndoHistory::randomGroupId("Add Model");

    const auto& model = assetStore.getAsset<Model>(modelName);
    Transform t{glm::vec3(0.0f), glm::quat(1.0f, 0.0f, 0.0f, 0.0f), glm::vec3(1.0f)};
    Renderer r{model.meshName, model.materialName, std::nullopt, true};

    EntityHandle handle = scene_.createObject(groupId);
    RuntimeGameObject obj = scene_.getObject(handle);
    obj.addComponent(Metadata{modelName}, groupId);
    obj.addComponent(t, groupId);
    obj.addComponent(r, groupId);
}

void SceneQuickActions::createPrimitiveGeometry(const std::string& meshName,
                                                const std::string& objectName,
                                                const std::string& mutationGroup) {
    ensurePrimitiveExists(meshName);

    EntityHandle handle = scene_.createObject(mutationGroup);
    RuntimeGameObject obj = scene_.getObject(handle);
    obj.addComponent(Metadata{objectName}, mutationGroup);
    obj.addComponent(Transform{glm::vec3(0.0f), glm::quat(1.0f, 0.0f, 0.0f, 0.0f), glm::vec3(1.0f)}, mutationGroup);
    obj.addComponent(Renderer{meshName, SOLID_COLOR_MATERIAL, std::nullopt, true}, mutationGroup);
}

void SceneQuickActions::ensurePrimitiveExists(const std::string& meshName) {
    if (assetStore.exists(meshName)) return;
    if (meshName == "Cube.mesh") {
        assetStore.createAssetFile<Mesh>(AssetPrefabs::cube(meshName));
    } else if (meshName == "Plane.mesh") {
        assetStore.createAssetFile<Mesh>(AssetPrefabs::plane(meshName));
    } else if (meshName.find("Sphere_") == 0) {
        size_t underscorePos = meshName.find('_');
        size_t dotPos = meshName.find('.');
        uint32_t resolution = std::stoul(meshName.substr(underscorePos + 1, dotPos - underscorePos - 1));
        assetStore.createAssetFile<Mesh>(AssetPrefabs::sphere(resolution, meshName));
    } else {
        throw std::runtime_error("Unknown mesh name: " + meshName);
    }
}

void SceneQuickActions::renameObject(EntityHandle entity, const std::string& newName) {
    auto groupId = UndoHistory::randomGroupId("Rename Object");
    scene_.getObject(entity).mutateComponent<Metadata>([newName](Metadata& m) { m.displayName = newName; }, groupId);
}

void SceneQuickActions::groupSelected(const std::string& groupName) {
    auto ids = editorSelection_.getSelectedEntityIds();
    if (ids.empty()) return;
    auto mutationGroup = UndoHistory::randomGroupId("Group Objects");

    // Create an empty group entity
    EntityHandle groupHandle = scene_.createObject(mutationGroup);
    scene_.getObject(groupHandle).addComponent<Metadata>(Metadata{groupName}, mutationGroup);

    // Build new hierarchy: remove each selected entity from wherever it is,
    // then nest them under the new group node.
    auto hierarchy = scene_.getHierarchy();
    std::vector<HierarchyNode> gathered;
    for (EntityHandle e: ids) {
        // Collect the subtree for e before removing it
        std::function<std::optional<HierarchyNode>(std::vector<HierarchyNode>&)> extract =
                [&](std::vector<HierarchyNode>& nodes) -> std::optional<HierarchyNode> {
            for (auto it = nodes.begin(); it != nodes.end(); ++it) {
                if (it->handle == e) {
                    auto node = std::move(*it);
                    nodes.erase(it);
                    return node;
                }
                if (auto found = extract(it->children)) return found;
            }
            return std::nullopt;
        };
        if (auto node = extract(hierarchy)) gathered.push_back(std::move(*node));
    }

    // The group node was appended to the top level by createObject; find and populate it.
    for (auto& node: hierarchy) {
        if (node.handle == groupHandle) {
            node.children = std::move(gathered);
            break;
        }
    }

    scene_.setHierarchy(std::move(hierarchy), mutationGroup);
    editorSelection_.clearSelection();
    editorSelection_.addToSelection(groupHandle, false);
}

void SceneQuickActions::ungroupNode(EntityHandle entity) {
    auto hierarchy = scene_.getHierarchy();

    // Find entity inside a parent's children list, extract the full subtree node,
    // insert it next to the parent in that same list, and return the parent handle.
    std::optional<EntityHandle> parentHandle;
    std::function<bool(std::vector<HierarchyNode>&)> extractAndInsert = [&](std::vector<HierarchyNode>& nodes) -> bool {
        for (auto& node: nodes) {
            auto& children = node.children;
            for (auto it = children.begin(); it != children.end(); ++it) {
                if (it->handle == entity) {
                    // Extract the full subtree (preserving children of entity).
                    HierarchyNode extracted = std::move(*it);
                    it = children.erase(it);
                    parentHandle = node.handle;
                    // Insert next to the parent in the grandparent's list (nodes).
                    // `nodes` here is the grandparent's children (or top-level list).
                    // We need to find the parent in nodes and insert after it.
                    for (auto pit = nodes.begin(); pit != nodes.end(); ++pit) {
                        if (pit->handle == node.handle) {
                            nodes.insert(std::next(pit), std::move(extracted));
                            return true;
                        }
                    }
                    // Parent not found in this list (shouldn't happen), append.
                    nodes.push_back(std::move(extracted));
                    return true;
                }
            }
            if (extractAndInsert(node.children)) return true;
        }
        return false;
    };

    // Try to find entity nested inside some parent. If it's top-level, nothing to do.
    // We need to search within each node's children, passing the parent list as context.
    std::function<bool(std::vector<HierarchyNode>&)> search = [&](std::vector<HierarchyNode>& nodes) -> bool {
        for (auto& node: nodes) {
            // Check if entity is a direct child of this node.
            for (auto it = node.children.begin(); it != node.children.end(); ++it) {
                if (it->handle == entity) {
                    HierarchyNode extracted = std::move(*it);
                    node.children.erase(it);
                    parentHandle = node.handle;
                    // Insert extracted after node in nodes (the grandparent list).
                    for (auto pit = nodes.begin(); pit != nodes.end(); ++pit) {
                        if (pit->handle == node.handle) {
                            nodes.insert(std::next(pit), std::move(extracted));
                            return true;
                        }
                    }
                    nodes.push_back(std::move(extracted));
                    return true;
                }
            }
            if (search(node.children)) return true;
        }
        return false;
    };

    if (!search(hierarchy)) return; // already top-level, nothing to do

    auto mutationGroup = UndoHistory::randomGroupId("Ungroup");

    // If the parent group is now empty, destroy it.
    std::function<bool(std::vector<HierarchyNode>&)> findEmpty = [&](std::vector<HierarchyNode>& nodes) -> bool {
        for (auto it = nodes.begin(); it != nodes.end(); ++it) {
            if (it->handle == *parentHandle && it->children.empty()) {
                nodes.erase(it);
                scene_.destroyObject(*parentHandle, mutationGroup);
                return true;
            }
            if (findEmpty(it->children)) return true;
        }
        return false;
    };
    findEmpty(hierarchy);

    scene_.setHierarchy(std::move(hierarchy), mutationGroup);
}

void SceneQuickActions::reparent(EntityHandle child, EntityHandle newParent) {
    auto hierarchy = scene_.getHierarchy();
    auto mutationGroup = UndoHistory::randomGroupId("Reparent");

    // Check if newParent is already a group (has children in the hierarchy).
    std::function<const HierarchyNode*(const std::vector<HierarchyNode>&)> findNode =
            [&](const std::vector<HierarchyNode>& nodes) -> const HierarchyNode* {
        for (const auto& node: nodes) {
            if (node.handle == newParent) return &node;
            if (auto* found = findNode(node.children)) return found;
        }
        return nullptr;
    };
    const HierarchyNode* parentNode = findNode(hierarchy);
    if (!parentNode) return;
    bool targetIsGroup = !parentNode->children.empty();

    // Extract the child node from its current location.
    std::optional<HierarchyNode> extracted;
    std::function<bool(std::vector<HierarchyNode>&)> extract = [&](std::vector<HierarchyNode>& nodes) -> bool {
        for (auto it = nodes.begin(); it != nodes.end(); ++it) {
            if (it->handle == child) {
                extracted = std::move(*it);
                nodes.erase(it);
                return true;
            }
            if (extract(it->children)) return true;
        }
        return false;
    };
    if (!extract(hierarchy) || !extracted) return;

    if (targetIsGroup) {
        // newParent is already a group — add child into it.
        std::function<bool(std::vector<HierarchyNode>&)> insert = [&](std::vector<HierarchyNode>& nodes) -> bool {
            for (auto& node: nodes) {
                if (node.handle == newParent) {
                    node.children.push_back(std::move(*extracted));
                    return true;
                }
                if (insert(node.children)) return true;
            }
            return false;
        };
        if (!insert(hierarchy)) return;
        scene_.setHierarchy(std::move(hierarchy), mutationGroup);
    } else {
        // newParent is a leaf — create a new group containing both.
        EntityHandle groupHandle = scene_.createObject(mutationGroup);
        scene_.getObject(groupHandle).addComponent<Metadata>(Metadata{"Group"}, mutationGroup);

        // Replace newParent in the hierarchy with the new group node.
        std::function<bool(std::vector<HierarchyNode>&)> replaceWithGroup =
                [&](std::vector<HierarchyNode>& nodes) -> bool {
            for (auto it = nodes.begin(); it != nodes.end(); ++it) {
                if (it->handle == newParent) {
                    HierarchyNode group{groupHandle, {std::move(*it), std::move(*extracted)}};
                    *it = std::move(group);
                    return true;
                }
                if (replaceWithGroup(it->children)) return true;
            }
            return false;
        };
        if (!replaceWithGroup(hierarchy)) return;
        scene_.setHierarchy(std::move(hierarchy), mutationGroup);
    }
}

void SceneQuickActions::reorder(EntityHandle dragged, EntityHandle anchor, bool insertBefore) {
    if (dragged == anchor) return;
    auto hierarchy = scene_.getHierarchy();

    // Extract the dragged node from wherever it is.
    std::optional<HierarchyNode> extracted;
    std::function<bool(std::vector<HierarchyNode>&)> extract = [&](std::vector<HierarchyNode>& nodes) -> bool {
        for (auto it = nodes.begin(); it != nodes.end(); ++it) {
            if (it->handle == dragged) {
                extracted = std::move(*it);
                nodes.erase(it);
                return true;
            }
            if (extract(it->children)) return true;
        }
        return false;
    };
    if (!extract(hierarchy) || !extracted) return;

    // Find the anchor in the sibling list and insert before/after it.
    std::function<bool(std::vector<HierarchyNode>&)> insert = [&](std::vector<HierarchyNode>& nodes) -> bool {
        for (auto it = nodes.begin(); it != nodes.end(); ++it) {
            if (it->handle == anchor) {
                auto pos = insertBefore ? it : std::next(it);
                nodes.insert(pos, std::move(*extracted));
                return true;
            }
            if (insert(it->children)) return true;
        }
        return false;
    };
    if (!insert(hierarchy)) return;

    scene_.setHierarchy(std::move(hierarchy), UndoHistory::randomGroupId("Reorder"));
}
