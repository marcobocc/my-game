#include "SceneQuickActions.hpp"

void SceneQuickActions::deleteSelection() {
    auto ids = editorSelection_.getSelectedEntityIds();
    if (ids.empty()) return;
    auto groupId = UndoHistory::randomGroupId("Delete Selection");
    for (EntityHandle e: ids)
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
