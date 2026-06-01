#include "RuntimeScene.hpp"
#include "modules/scene/components/BoxCollider.hpp"
#include "modules/scene/components/Camera.hpp"
#include "modules/scene/components/Light.hpp"
#include "modules/scene/components/Metadata.hpp"
#include "modules/scene/components/Renderer.hpp"
#include "modules/scene/components/Transform.hpp"

RuntimeGameObject RuntimeScene::getObject(EntityHandle e) const {
    if (world_.hasActor(e)) return RuntimeGameObject(world_, undo_, e);
    throw std::runtime_error("GameObject does not exist");
}

SceneDTO RuntimeScene::snapshotScene() const {
    SceneDTO scene;
    for (const auto& actor: world_.getActors()) {
        scene.objects.push_back(snapshotObject(actor->handle()));
    }
    return scene;
}

GameObjectDTO RuntimeScene::snapshotObject(EntityHandle e) const {
    GameObjectDTO dto(e);
    const Actor* actor = world_.getActor(e);
    if (!actor) return dto;
    auto tryAdd = [&]<typename T>() {
        if (const T* c = actor->getComponent<T>()) dto.components.emplace_back(*c);
    };
    tryAdd.operator()<Metadata>();
    tryAdd.operator()<Transform>();
    tryAdd.operator()<Camera>();
    tryAdd.operator()<Renderer>();
    tryAdd.operator()<BoxCollider>();
    tryAdd.operator()<Light>();
    return dto;
}

void RuntimeScene::clearScene(const std::string& mutationGroup) {
    SceneDTO before = snapshotScene();
    world_.clear();
    nextHandle_ = 0;
    undo_.push([this, before] { restore_Impl(before); },
               [this] {
                   world_.clear();
                   nextHandle_ = 0;
               },
               "Clear Scene",
               mutationGroup);
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
    EntityHandle handle = nextHandle_++;
    world_.addActor(handle);
    undo_.push([this, handle] { world_.removeActor(handle); },
               [this, handle] { world_.addActor(handle); },
               "Create GameObject",
               mutationGroup);
    return handle;
}

void RuntimeScene::destroyObject(EntityHandle e, const std::string& mutationGroup) {
    if (!world_.hasActor(e)) return;
    auto dto = snapshotObject(e);
    world_.removeActor(e);
    undo_.push([this, dto] { restore_Impl(dto); },
               [this, e] { world_.removeActor(e); },
               "Destroy GameObject",
               mutationGroup);
}

void RuntimeScene::restoreObject(const GameObjectDTO& dto, const std::string& mutationGroup) {
    std::optional<GameObjectDTO> before;
    if (world_.hasActor(dto.handle)) before = snapshotObject(dto.handle);
    restore_Impl(dto);
    undo_.push(
            [this, before, handle = dto.handle] {
                if (before)
                    restore_Impl(*before);
                else
                    world_.removeActor(handle);
            },
            [this, dto] { restore_Impl(dto); },
            "Restore GameObject",
            mutationGroup);
}

void RuntimeScene::restore_Impl(const SceneDTO& dto) {
    world_.clear();
    nextHandle_ = 0;
    for (const auto& objDto: dto.objects) {
        restore_Impl(objDto);
        if (objDto.handle >= nextHandle_) nextHandle_ = objDto.handle + 1;
    }
}

void RuntimeScene::restore_Impl(const GameObjectDTO& dto) {
    if (world_.hasActor(dto.handle)) world_.removeActor(dto.handle);
    world_.addActor(dto.handle);
    Actor* actor = world_.getActor(dto.handle);
    for (auto& c: dto.components) {
        std::visit(
                [&]<typename T0>(T0&& comp) {
                    using T = std::decay_t<T0>;
                    if (actor->getComponent<T>()) actor->removeComponent(actor->getComponent<T>());
                    actor->addComponent<T>(comp);
                },
                c);
    }
}
