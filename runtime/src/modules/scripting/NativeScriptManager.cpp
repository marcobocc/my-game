#include "NativeScriptManager.hpp"
#include <dlfcn.h>
#include <log4cxx/logger.h>
#include <stdexcept>

namespace {
    const log4cxx::LoggerPtr logger = log4cxx::Logger::getLogger("NativeScriptManager");
}

NativeScriptManager::NativeScriptManager(ScriptInstanceContext ctx) : ctx_(ctx) {}

NativeScriptManager::~NativeScriptManager() { unloadAll(); }

void NativeScriptManager::loadScript(const std::string& name, const std::filesystem::path& path) {
    if (scripts_.contains(name)) return;

    void* handle = dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL);
    if (!handle) {
        throw std::runtime_error("dlopen failed for '" + name + "': " + dlerror());
    }

    auto* createFn = reinterpret_cast<CreateBehaviourFn>(dlsym(handle, "create_behaviour"));
    if (!createFn) {
        dlclose(handle);
        throw std::runtime_error("'" + name + "' is missing the create_behaviour symbol");
    }

    scripts_[name] = ScriptEntry{handle, createFn, path};
    LOG4CXX_INFO(logger, "Loaded script: " << name);
}

BehaviourComponent* NativeScriptManager::spawnInstance(const std::string& name, EntityHandle handle) {
    auto it = scripts_.find(name);
    if (it == scripts_.end()) {
        LOG4CXX_WARN(logger, "spawnInstance: script '" << name << "' is not loaded");
        return nullptr;
    }

    auto instance = std::unique_ptr<BehaviourComponent>(it->second.createFn(handle, &ctx_));
    instance->onStart();
    BehaviourComponent* raw = instance.get();
    instances_.push_back(std::move(instance));
    return raw;
}

void NativeScriptManager::tickAll(float dt) {
    for (auto& instance: instances_) {
        instance->onUpdate(dt);
    }
}

void NativeScriptManager::unloadAll() {
    instances_.clear();
    for (auto& [name, entry]: scripts_) {
        dlclose(entry.handle);
        LOG4CXX_INFO(logger, "Unloaded script: " << name);
    }
    scripts_.clear();
}
