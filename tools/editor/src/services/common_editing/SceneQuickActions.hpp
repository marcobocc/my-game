#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <optional>
#include <string>
#include "../ClipboardService.hpp"
#include "../EditorSelection.hpp"
#include "../UndoHistory.hpp"
#include "AssetStore.hpp"
#include "RuntimeScene.hpp"
#include "details/AssetPrefabs.hpp"
#include "modules/asset_management/asset_types/Mesh.hpp"
#include "modules/asset_management/asset_types/Model.hpp"
#include "modules/scene/components/Light.hpp"
#include "modules/scene/components/Metadata.hpp"
#include "modules/scene/components/Renderer.hpp"
#include "modules/scene/components/Transform.hpp"
#include "transport/GameObjectDTO.hpp"

class SceneQuickActions {
public:
    SceneQuickActions(RuntimeScene& scene,
                      EditorSelection& editorSelection,
                      ClipboardService& clipboard,
                      AssetStore& assetStore) :
        scene_(scene),
        editorSelection_(editorSelection),
        editorClipboard_(clipboard),
        assetStore(assetStore) {}

    // --------------------------------------------------------------------------------
    // Clipboard Actions
    // --------------------------------------------------------------------------------
    void deleteSelection();
    void copySelection();
    void cutSelected();
    void pasteSelection();
    void duplicateSelection();

    // --------------------------------------------------------------------------------
    // Object Mutations
    // --------------------------------------------------------------------------------
    void renameObject(EntityHandle entity, const std::string& newName);

    // --------------------------------------------------------------------------------
    // Hierarchy
    // --------------------------------------------------------------------------------
    // Wraps all selected entities under a new named group node.
    void groupSelected(const std::string& groupName = "Group");
    // Dissolves a group node: its children become siblings at the same level.
    void ungroupNode(EntityHandle groupEntity);
    // Moves `child` under `newParent` in the hierarchy tree.
    void reparent(EntityHandle child, EntityHandle newParent);
    // Moves `dragged` to be a sibling of `anchor`, inserted before or after it.
    void reorder(EntityHandle dragged, EntityHandle anchor, bool insertBefore);

    // --------------------------------------------------------------------------------
    // Prefab Objects
    // --------------------------------------------------------------------------------
    void createEmptyObject();
    void createLight();
    void createCube();
    void createPlane();
    void createSphere(uint32_t resolution);
    void createCapsule(uint32_t resolution);
    void addModel(const std::string& modelName);

private:
    void createPrimitiveGeometry(const std::string& meshName,
                                 const std::string& objectName,
                                 const std::string& mutationGroup);
    void ensurePrimitiveExists(const std::string& meshName);

    RuntimeScene& scene_;
    EditorSelection& editorSelection_;
    ClipboardService& editorClipboard_;
    AssetStore& assetStore;
};
