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
    // Prefab Objects
    // --------------------------------------------------------------------------------
    void createEmptyObject();
    void createLight();
    void createCube();
    void createPlane();
    void createSphere(uint32_t resolution);
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
