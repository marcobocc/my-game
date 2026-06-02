#include "NativeScriptManager.hpp"
#include <dlfcn.h>
#include <log4cxx/logger.h>
#include <stdexcept>

namespace {
    const log4cxx::LoggerPtr logger = log4cxx::Logger::getLogger("NativeScriptManager");
}

NativeScriptManager::NativeScriptManager(ScriptInstanceContext ctx) : ctx_(ctx) {
    ctx_.findScriptInstance = [this](std::type_index type, unsigned int handle) -> void* {
        auto it = entityInstances_.find(handle);
        if (it == entityInstances_.end()) return nullptr;
        for (BehaviourComponent* instance: it->second) {
            if (std::type_index(typeid(*instance)) == type) return instance;
        }
        return nullptr;
    };
}

NativeScriptManager::~NativeScriptManager() { unload(); }

void NativeScriptManager::loadDylib(const std::filesystem::path& path) {
    unload();
    dylibHandle_ = dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL);
    if (!dylibHandle_) throw std::runtime_error("dlopen failed for scripts.dylib: " + std::string(dlerror()));
    LOG4CXX_INFO(logger, "Loaded scripts.dylib");
}

BehaviourComponent* NativeScriptManager::spawnInstance(const std::string& className, EntityHandle handle) {
    if (!dylibHandle_) {
        LOG4CXX_WARN(logger, "spawnInstance: dylib not loaded");
        return nullptr;
    }

    auto it = factories_.find(className);
    if (it == factories_.end()) {
        std::string symbol = "create_" + className;
        auto* fn = reinterpret_cast<CreateBehaviourFn>(dlsym(dylibHandle_, symbol.c_str()));
        if (!fn) {
            LOG4CXX_WARN(logger, "spawnInstance: symbol '" << symbol << "' not found in scripts.dylib");
            return nullptr;
        }
        factories_[className] = fn;
        it = factories_.find(className);
    }

    auto instance = std::unique_ptr<BehaviourComponent>(it->second(handle, &ctx_));
    instance->onStart();
    BehaviourComponent* raw = instance.get();
    entityInstances_[handle].push_back(raw);
    instances_.push_back(std::move(instance));
    return raw;
}

void NativeScriptManager::tickAll(float dt) {
    for (auto& instance: instances_)
        instance->onUpdate(dt);
}

void NativeScriptManager::unload() {
    instances_.clear();
    entityInstances_.clear();
    factories_.clear();
    if (dylibHandle_) {
        dlclose(dylibHandle_);
        dylibHandle_ = nullptr;
        LOG4CXX_INFO(logger, "Unloaded scripts.dylib");
    }
}
