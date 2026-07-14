#include "SceneQuickActions.hpp"
#include <functional>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/quaternion.hpp>
#include <optional>
#include <unordered_set>
#include "core/scene/TransformUtils.hpp"
#include "physics/components/TerrainCollider.hpp"
#include "transport/SceneDTO.hpp"

void SceneQuickActions::deleteSelection() {
    auto ids = editorSelection_.getSelectedEntityIds();
    if (ids.empty()) return;
    auto groupId = UndoHistory::randomGroupId("Delete Selection");

    // Also collect all descendants of selected entities.
    auto hierarchy = scene_.getHierarchy();
    std::unordered_set<EntityHandle> toDestroy(ids.begin(), ids.end());

    std::function<void(const HierarchyNode&)> collectDescendants = [&](const HierarchyNode& node) {
        if (toDestroy.count(node.handle)) {
            for (const auto& child: node.children)
                collectDescendants(child);
        } else {
            for (const auto& child: node.children)
                collectDescendants(child);
        }
    };
    std::function<void(const HierarchyNode&)> markSubtree = [&](const HierarchyNode& node) {
        if (toDestroy.count(node.handle)) {
            std::function<void(const HierarchyNode&)> addAll = [&](const HierarchyNode& n) {
                toDestroy.insert(n.handle);
                for (const auto& c: n.children)
                    addAll(c);
            };
            addAll(node);
        } else {
            for (const auto& child: node.children)
                markSubtree(child);
        }
    };
    for (const auto& node: hierarchy)
        markSubtree(node);

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

void SceneQuickActions::createCapsule(uint32_t resolution) {
    auto groupId = UndoHistory::randomGroupId("Create Capsule");
    std::string meshName = "Capsule_" + std::to_string(resolution) + ".mesh";
    createPrimitiveGeometry(meshName, "Capsule", groupId);
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

void SceneQuickActions::createTerrain(uint32_t resolution, float worldSize) {
    auto groupId = UndoHistory::randomGroupId("Create Terrain");

    std::string meshName = "Terrain.mesh";
    int index = 2;
    while (assetStore.exists(meshName)) {
        meshName = "Terrain_" + std::to_string(index) + ".mesh";
        ++index;
    }
    assetStore.createAssetFile<Mesh>(AssetPrefabs::terrainMesh(resolution, worldSize, meshName), groupId);

    EntityHandle handle = scene_.createObject(groupId);
    RuntimeGameObject obj = scene_.getObject(handle);
    obj.addComponent(Metadata{"Terrain"}, groupId);
    obj.addComponent(Transform{glm::vec3(0.0f), glm::quat(1.0f, 0.0f, 0.0f, 0.0f), glm::vec3(1.0f)}, groupId);
    obj.addComponent(Renderer{meshName, SOLID_COLOR_MATERIAL, std::nullopt, true}, groupId);
    obj.addComponent(TerrainCollider{}, groupId);
}

void SceneQuickActions::enableTerrainPainting(EntityHandle entity, uint32_t splatMapResolution) {
    const Actor* actor = scene_.getWorld().getActor(entity);
    if (!actor) return;
    const Renderer* renderer = actor->getComponent<Renderer>();
    if (!renderer) return;
    if (assetStore.exists(renderer->materialName) &&
        assetStore.getAsset<Material>(renderer->materialName).isTerrainBlend())
        return;

    auto groupId = UndoHistory::randomGroupId("Enable Terrain Painting");

    std::string splatMapName = "Terrain.splatmap.png";
    int index = 2;
    while (assetStore.exists(splatMapName)) {
        splatMapName = "Terrain_" + std::to_string(index) + ".splatmap.png";
        ++index;
    }
    assetStore.createAssetFile<Texture>(AssetPrefabs::splatMap(splatMapResolution, splatMapName), groupId);

    std::string materialName = "Terrain.mat";
    index = 2;
    while (assetStore.exists(materialName)) {
        materialName = "Terrain_" + std::to_string(index) + ".mat";
        ++index;
    }
    assetStore.createAssetFile<Material>(AssetPrefabs::terrainBlend(materialName, splatMapName), groupId);

    scene_.getObject(entity).mutateComponent<Renderer>([materialName](Renderer& r) { r.materialName = materialName; },
                                                       groupId);
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
    } else if (meshName.find("Capsule_") == 0) {
        size_t underscorePos = meshName.find('_');
        size_t dotPos = meshName.find('.');
        uint32_t resolution = std::stoul(meshName.substr(underscorePos + 1, dotPos - underscorePos - 1));
        assetStore.createAssetFile<Mesh>(AssetPrefabs::capsule(resolution, meshName));
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

    // Create an empty group entity with a Transform so children can be relative to it.
    EntityHandle groupHandle = scene_.createObject(mutationGroup);
    scene_.getObject(groupHandle).addComponent<Metadata>(Metadata{groupName}, mutationGroup);
    scene_.getObject(groupHandle).addComponent<Transform>(Transform{}, mutationGroup);

    // Build new hierarchy: remove each selected entity from wherever it is,
    // then nest them under the new group node.
    auto hierarchy = scene_.getHierarchy();
    std::vector<HierarchyNode> gathered;
    for (EntityHandle e: ids) {
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

        // Set Transform.parent so the child's transform is relative to the group.
        scene_.getObject(e).mutateComponent<Transform>([groupHandle](Transform& t) { t.parent = groupHandle; },
                                                       mutationGroup);
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
    std::optional<EntityHandle> parentHandle;

    std::function<bool(std::vector<HierarchyNode>&)> search = [&](std::vector<HierarchyNode>& nodes) -> bool {
        for (auto& node: nodes) {
            for (auto it = node.children.begin(); it != node.children.end(); ++it) {
                if (it->handle == entity) {
                    HierarchyNode extracted = std::move(*it);
                    node.children.erase(it);
                    parentHandle = node.handle;
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

    // Clear Transform.parent so the entity is no longer relative to its old parent.
    scene_.getObject(entity).mutateComponent<Transform>([](Transform& t) { t.parent = INVALID_ENTITY_HANDLE; },
                                                        mutationGroup);

    // If the parent group is now childless, destroy it.
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
    if (child == newParent) return;
    auto hierarchy = scene_.getHierarchy();
    auto mutationGroup = UndoHistory::randomGroupId("Reparent");

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

    // Insert child under newParent.
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

    // Preserve world position: compute child's current world matrix, then express it
    // in the new parent's local space so the world transform stays the same.
    const World& world = scene_.getWorld();
    const Actor* childActor = world.getActor(child);
    const Actor* parentActor = world.getActor(newParent);
    if (childActor && parentActor) {
        const Transform* childTransform = childActor->getComponent<Transform>();
        const Transform* parentTransform = parentActor->getComponent<Transform>();
        if (childTransform && parentTransform) {
            glm::mat4 childWorld = TransformUtils::resolveWorldMatrix(*childTransform, world);
            glm::mat4 parentWorld = TransformUtils::resolveWorldMatrix(*parentTransform, world);
            glm::mat4 localMat = glm::inverse(parentWorld) * childWorld;

            glm::vec3 newPos = glm::vec3(localMat[3]);
            glm::vec3 newScale = {glm::length(glm::vec3(localMat[0])),
                                  glm::length(glm::vec3(localMat[1])),
                                  glm::length(glm::vec3(localMat[2]))};
            glm::mat3 rotMat = {glm::vec3(localMat[0]) / newScale.x,
                                glm::vec3(localMat[1]) / newScale.y,
                                glm::vec3(localMat[2]) / newScale.z};
            glm::quat newRot = glm::quat_cast(rotMat);

            scene_.getObject(child).mutateComponent<Transform>(
                    [newParent, newPos, newRot, newScale](Transform& t) {
                        t.parent = newParent;
                        t.position = newPos;
                        t.rotation = newRot;
                        t.scale = newScale;
                    },
                    mutationGroup);
        }
    }

    scene_.setHierarchy(std::move(hierarchy), mutationGroup);
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

    // Find the anchor and its parent so we can update Transform.parent.
    EntityHandle newParent = INVALID_ENTITY_HANDLE;
    std::function<bool(std::vector<HierarchyNode>&, EntityHandle)> insert = [&](std::vector<HierarchyNode>& nodes,
                                                                                EntityHandle parentHandle) -> bool {
        for (auto it = nodes.begin(); it != nodes.end(); ++it) {
            if (it->handle == anchor) {
                newParent = parentHandle;
                auto pos = insertBefore ? it : std::next(it);
                nodes.insert(pos, std::move(*extracted));
                return true;
            }
            if (insert(it->children, it->handle)) return true;
        }
        return false;
    };
    if (!insert(hierarchy, INVALID_ENTITY_HANDLE)) return;

    auto mutationGroup = UndoHistory::randomGroupId("Reorder");
    scene_.getObject(dragged).mutateComponent<Transform>([newParent](Transform& t) { t.parent = newParent; },
                                                         mutationGroup);
    scene_.setHierarchy(std::move(hierarchy), mutationGroup);
}
