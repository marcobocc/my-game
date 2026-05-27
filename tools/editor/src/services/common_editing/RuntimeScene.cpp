#include "RuntimeScene.hpp"

RuntimeGameObject RuntimeScene::getObject(EntityHandle e) const {
    if (store_.exists(e)) return RuntimeGameObject(store_, undo_, e);
    throw std::runtime_error("GameObject does not exist");
}

SceneDTO RuntimeScene::snapshotScene() const {
    SceneDTO scene;
    for (EntityHandle e: store_.getEntities()) {
        GameObjectDTO objDto = snapshotObject(e);
        scene.objects.push_back(objDto);
    }
    return scene;
}

GameObjectDTO RuntimeScene::snapshotObject(EntityHandle e) const {
    GameObjectDTO dto(e);
    store_.forEachComponent(e, [&](auto& comp) { dto.components.emplace_back(comp); });
    return dto;
}

void RuntimeScene::clearScene(const std::string& mutationGroup) {
    SceneDTO before = snapshotScene();
    store_.clear();
    undo_.push([this, before] { restore_Impl(before); }, [this] { store_.clear(); }, "Clear Scene", mutationGroup);
}

void RuntimeScene::restoreScene(const SceneDTO& dto, const std::string& mutationGroup) {
    SceneDTO before = snapshotScene();
    restore_Impl(dto);
    undo_.push([this, before] { restore_Impl(before); },
               [this, dto] { restore_Impl(dto); },
               "Restore Scene",
               mutationGroup);
}

EntityHandle RuntimeScene::createObject(const std::string& mutationGroup) {
    EntityHandle handle = store_.createEntity();
    undo_.push([this, handle] { store_.destroyEntity(handle); },
               [this, handle] { store_.createEntity(handle); },
               "Create GameObject",
               mutationGroup);
    return handle;
}

void RuntimeScene::destroyObject(EntityHandle e, const std::string& mutationGroup) {
    if (!store_.exists(e)) return;
    auto dto = snapshotObject(e);
    store_.destroyEntity(e);
    undo_.push([this, dto] { restore_Impl(dto); },
               [this, e] { store_.destroyEntity(e); },
               "Destroy GameObject",
               mutationGroup);
}

void RuntimeScene::restoreObject(const GameObjectDTO& dto, const std::string& mutationGroup) {
    std::optional<GameObjectDTO> before;
    if (store_.exists(dto.handle)) before = snapshotObject(dto.handle);
    restore_Impl(dto);
    undo_.push(
            [this, before, handle = dto.handle] {
                if (before)
                    restore_Impl(*before);
                else
                    store_.destroyEntity(handle);
            },
            [this, dto] { restore_Impl(dto); },
            "Restore GameObject",
            mutationGroup);
}

void RuntimeScene::restore_Impl(const SceneDTO& dto) {
    store_.clear();
    for (const auto& objDto: dto.objects) {
        restore_Impl(objDto);
    }
}

void RuntimeScene::restore_Impl(const GameObjectDTO& dto) {
    if (store_.exists(dto.handle)) store_.destroyEntity(dto.handle);
    store_.createEntity(dto.handle);
    for (auto& c: dto.components) {
        std::visit(
                [&]<typename T0>(T0&& comp) {
                    using T = std::decay_t<T0>;
                    if (store_.getComponent<T>(dto.handle)) store_.removeComponent<T>(dto.handle);
                    store_.upsertComponent<T>(dto.handle, comp);
                },
                c);
    }
}
