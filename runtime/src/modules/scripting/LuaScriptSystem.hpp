#pragma once
#include <filesystem>
#include <log4cxx/logger.h>
#include <nlohmann/json.hpp>
#include <sol/sol.hpp>
#include <string>
#include <unordered_map>
#include <vector>
#include "LuaBindings.hpp"
#include "LuaInput.hpp"
#include "LuaWorld.hpp"
#include "modules/input/InputSystem.hpp"
#include "modules/scene/EntityHandle.hpp"
#include "modules/scene/World.hpp"
#include "modules/scene/components/BehaviourScript.hpp"

class LuaScriptSystem {
public:
    void init(World& world, InputSystem& input, const std::filesystem::path& scriptsDir);
    void update(float dt);
    void callOnCollision(EntityHandle self, EntityHandle other);

private:
    struct ScriptChunk {
        sol::table proto;
        std::filesystem::file_time_type lastWriteTime;
    };

    struct ScriptInstance {
        EntityHandle entity;
        std::string scriptName;
        sol::table self;
    };

    sol::state lua_;
    LuaWorld* worldFacade_ = nullptr;
    LuaInput* inputFacade_ = nullptr;
    std::filesystem::path scriptsDir_;
    World* world_ = nullptr;
    log4cxx::LoggerPtr logger_;

    std::unordered_map<std::string, ScriptChunk> chunks_;
    std::vector<ScriptInstance> instances_;

    // Heap-allocated facades so pointers stay stable after init
    std::unique_ptr<LuaWorld> ownedWorld_;
    std::unique_ptr<LuaInput> ownedInput_;

    ScriptChunk& loadChunk(const std::string& scriptName);
    sol::table instantiate(sol::table proto,
                           EntityHandle entity,
                           const std::unordered_map<std::string, nlohmann::json>& propertyValues);
    void callOnStart(ScriptInstance& inst);
    void reload(ScriptInstance& inst);
    void checkHotReload();

    std::filesystem::path pathFor(const std::string& name) const { return scriptsDir_ / (name + ".lua"); }

    template<typename Fn>
    void safeCall(const char* context, Fn&& fn) {
        try {
            fn();
        } catch (const sol::error& e) {
            LOG4CXX_ERROR(logger_, "[" << context << "] Lua error: " << e.what());
        } catch (const std::exception& e) {
            LOG4CXX_ERROR(logger_, "[" << context << "] Error: " << e.what());
        }
    }
};
