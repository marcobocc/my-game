#include "RuntimeScene.hpp"
#include "../../../../../runtime/src/animation/components/Animator.hpp"
#include "../../../../../runtime/src/core/components/Metadata.hpp"
#include "../../../../../runtime/src/core/components/Transform.hpp"
#include "../../../../../runtime/src/graphics/components/Camera.hpp"
#include "../../../../../runtime/src/graphics/components/Light.hpp"
#include "../../../../../runtime/src/graphics/components/ParticleEmitter.hpp"
#include "../../../../../runtime/src/graphics/components/Renderer.hpp"
#include "../../../../../runtime/src/graphics/components/TextComponent.hpp"
#include "../../../../../runtime/src/physics/components/CapsuleCollider.hpp"
#include "../../../../../runtime/src/scripting/components/BehaviourScript.hpp"
#include "physics/components/BoxCollider.hpp"

RuntimeGameObject RuntimeScene::getObject(EntityHandle e) const {
    if (world_.hasActor(e)) return RuntimeGameObject(world_, undo_, e);
    throw std::runtime_error("GameObject does not exist");
}

SceneDTO RuntimeScene::snapshotScene() const {
    SceneDTO scene;
    for (const auto& actor: world_.getActors())
        scene.objects.push_back(snapshotObject(actor->handle()));
    scene.hierarchy = hierarchy_;
    pruneHierarchy(scene.hierarchy);
    if (const Actor* cam = world_.getActiveCamera()) scene.activeCameraHandle = cam->handle();
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
    tryAdd.operator()<CapsuleCollider>();
    tryAdd.operator()<Light>();
    tryAdd.operator()<ParticleEmitter>();
    for (const BehaviourScript* s: actor->getComponents<BehaviourScript>())
        dto.components.emplace_back(*s);
    tryAdd.operator()<Animator>();
    tryAdd.operator()<TextComponent>();
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
    hierarchy_.push_back({handle, {}});
    auto prevHierarchy = hierarchy_;
    prevHierarchy.pop_back();
    undo_.push(
            [this, handle, prevHierarchy] {
                world_.removeActor(handle);
                hierarchy_ = prevHierarchy;
            },
            [this, handle] {
                world_.addActor(handle);
                hierarchy_.push_back({handle, {}});
            },
            "Create GameObject",
            mutationGroup);
    return handle;
}

void RuntimeScene::destroyObject(EntityHandle e, const std::string& mutationGroup) {
    if (!world_.hasActor(e)) return;
    auto dto = snapshotObject(e);
    auto prevHierarchy = hierarchy_;
    world_.removeActor(e);
    removeFromHierarchy(hierarchy_, e);
    undo_.push(
            [this, dto, prevHierarchy] {
                restore_Impl(dto);
                hierarchy_ = prevHierarchy;
            },
            [this, e] {
                world_.removeActor(e);
                removeFromHierarchy(hierarchy_, e);
            },
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

EntityHandle RuntimeScene::instantiatePrefab(const SceneDTO& prefab, const std::string& mutationGroup) {
    if (prefab.objects.empty()) return INVALID_ENTITY_HANDLE;

    // Remap prefab-local handles to fresh scene handles.
    std::unordered_map<EntityHandle, EntityHandle> remap;
    std::vector<GameObjectDTO> remapped;
    for (const auto& obj: prefab.objects) {
        EntityHandle newHandle = nextHandle_++;
        remap[obj.handle] = newHandle;
        GameObjectDTO copy = obj;
        copy.handle = newHandle;
        remapped.push_back(std::move(copy));
    }

    // Remap hierarchy node handles recursively.
    std::function<HierarchyNode(const HierarchyNode&)> remapNode = [&](const HierarchyNode& n) {
        HierarchyNode out;
        auto it = remap.find(n.handle);
        out.handle = (it != remap.end()) ? it->second : n.handle;
        for (const auto& child: n.children)
            out.children.push_back(remapNode(child));
        return out;
    };

    // Identify root handle (first hierarchy root, or first object).
    EntityHandle rootHandle = remapped.front().handle;
    std::vector<HierarchyNode> subtree;
    if (!prefab.hierarchy.empty()) {
        subtree.push_back(remapNode(prefab.hierarchy.front()));
        rootHandle = subtree.front().handle;
    } else {
        for (const auto& obj: remapped)
            subtree.push_back({obj.handle, {}});
    }

    for (const auto& obj: remapped)
        restore_Impl(obj);
    for (const auto& node: subtree)
        hierarchy_.push_back(node);

    std::vector<EntityHandle> newHandles;
    for (const auto& obj: remapped)
        newHandles.push_back(obj.handle);

    undo_.push(
            [this, newHandles] {
                for (auto h: newHandles) {
                    world_.removeActor(h);
                    removeFromHierarchy(hierarchy_, h);
                }
            },
            [this, remapped, subtree] {
                for (const auto& obj: remapped)
                    restore_Impl(obj);
                for (const auto& node: subtree)
                    hierarchy_.push_back(node);
            },
            "Instantiate Prefab",
            mutationGroup);

    return rootHandle;
}

void RuntimeScene::restore_Impl(const SceneDTO& dto) {
    world_.clear();
    nextHandle_ = 0;
    for (const auto& objDto: dto.objects) {
        restore_Impl(objDto);
        if (objDto.handle >= nextHandle_) nextHandle_ = objDto.handle + 1;
    }
    hierarchy_ = dto.hierarchy;
    // If the saved DTO has no hierarchy (old scene files), build a flat one.
    if (hierarchy_.empty()) {
        for (const auto& objDto: dto.objects)
            hierarchy_.push_back({objDto.handle, {}});
    }
    if (dto.activeCameraHandle) world_.setActiveCamera(*dto.activeCameraHandle);
}

void RuntimeScene::setHierarchy(std::vector<HierarchyNode> newHierarchy, const std::string& mutationGroup) {
    auto prev = hierarchy_;
    hierarchy_ = std::move(newHierarchy);
    undo_.push([this, prev] { hierarchy_ = prev; },
               [this, newH = hierarchy_] { hierarchy_ = newH; },
               "Rearrange Hierarchy",
               mutationGroup);
}

bool RuntimeScene::removeFromHierarchy(std::vector<HierarchyNode>& nodes, EntityHandle e) {
    for (auto it = nodes.begin(); it != nodes.end(); ++it) {
        if (it->handle == e) {
            nodes.erase(it);
            return true;
        }
        if (removeFromHierarchy(it->children, e)) return true;
    }
    return false;
}

bool RuntimeScene::addChildToNode(std::vector<HierarchyNode>& nodes, EntityHandle parent, EntityHandle child) {
    for (auto& node: nodes) {
        if (node.handle == parent) {
            node.children.push_back({child, {}});
            return true;
        }
        if (addChildToNode(node.children, parent, child)) return true;
    }
    return false;
}

void RuntimeScene::pruneHierarchy(std::vector<HierarchyNode>& nodes) const {
    for (auto it = nodes.begin(); it != nodes.end();) {
        if (!world_.hasActor(it->handle)) {
            it = nodes.erase(it);
            continue;
        }
        pruneHierarchy(it->children);
        ++it;
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
                    // BehaviourScript is multi-instance; all others are singletons.
                    if constexpr (!std::is_same_v<T, BehaviourScript>)
                        if (actor->getComponent<T>()) actor->removeComponent(actor->getComponent<T>());
                    actor->addComponent<T>(comp);
                },
                c);
    }
}
