#pragma once
#include <filesystem>
#include <memory>
#include <string>
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

    // Load the .dylib and register its factory function. No-op if already loaded.
    void loadScript(const std::string& name, const std::filesystem::path& path);

    // Instantiate a behaviour for an actor and call onStart(). Returns nullptr if script not loaded.
    BehaviourComponent* spawnInstance(const std::string& name, EntityHandle handle);

    // Call onUpdate(dt) on all live instances.
    void tickAll(float dt);

private:
    void unloadAll();

    struct ScriptEntry {
        void* handle = nullptr;
        CreateBehaviourFn createFn = nullptr;
        std::filesystem::path path;
    };

    ScriptInstanceContext ctx_;
    std::unordered_map<std::string, ScriptEntry> scripts_;
    std::vector<std::unique_ptr<BehaviourComponent>> instances_;
};
