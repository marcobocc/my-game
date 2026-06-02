#pragma once
#include <filesystem>
#include <memory>
#include <string>
#include <typeindex>
#include <unordered_map>
#include <vector>
#include "BehaviourComponent.hpp"
#include "ScriptInstanceContext.hpp"
#include "modules/scene/EntityHandle.hpp"

using CreateBehaviourFn = BehaviourComponent* (*) (EntityHandle, ScriptInstanceContext*);

class NativeScriptManager {
public:
    explicit NativeScriptManager(ScriptInstanceContext ctx);
    ~NativeScriptManager();

    NativeScriptManager(const NativeScriptManager&) = delete;
    NativeScriptManager& operator=(const NativeScriptManager&) = delete;

    // Load scripts.dylib and register all create_<ClassName> factory symbols found in it.
    void loadDylib(const std::filesystem::path& path);

    // Instantiate a behaviour by class name and call onStart(). Returns nullptr if class not found.
    BehaviourComponent* spawnInstance(const std::string& className, EntityHandle handle);

    // Call onUpdate(dt) on all live instances.
    void tickAll(float dt);

private:
    void unload();

    ScriptInstanceContext ctx_;
    void* dylibHandle_ = nullptr;
    std::unordered_map<std::string, CreateBehaviourFn> factories_;
    std::vector<std::unique_ptr<BehaviourComponent>> instances_;
    std::unordered_map<EntityHandle, std::vector<BehaviourComponent*>> entityInstances_;
};
