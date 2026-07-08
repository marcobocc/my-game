#pragma once
#include <filesystem>
#include <log4cxx/logger.h>
#include <nlohmann/json.hpp>
#include <sol/sol.hpp>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "api/LuaDebugDraw.hpp"
#include "api/LuaInput.hpp"
#include "api/LuaWorld.hpp"
#include "modules/debug/DebugDraw.hpp"
#include "modules/input/InputSystem.hpp"
#include "modules/scene/EntityHandle.hpp"
#include "modules/scene/World.hpp"

class LuaScriptSystem {
public:
    void init(World& world, InputSystem& input, DebugDraw& debugDraw);
    void registerScript(const std::string& name, const std::filesystem::path& path);
    void startInstances();
    void update(float dt);
    void callOnCollision(EntityHandle self, EntityHandle other);

private:
    struct ScriptChunk {
        sol::table proto;
        std::filesystem::file_time_type lastWriteTime;
        std::unordered_set<std::string> entityProps;
    };

    struct ScriptInstance {
        EntityHandle entity;
        std::string scriptName;
        sol::table self;
    };

    sol::state lua_;
    LuaWorld* worldFacade_ = nullptr;
    LuaInput* inputFacade_ = nullptr;
    LuaDebugDraw* debugDrawFacade_ = nullptr;
    std::unordered_map<std::string, std::filesystem::path> registeredScripts_;
    World* world_ = nullptr;
    log4cxx::LoggerPtr logger_;

    std::unordered_map<std::string, ScriptChunk> chunks_;
    std::vector<ScriptInstance> instances_;

    std::unique_ptr<LuaWorld> ownedWorld_;
    std::unique_ptr<LuaInput> ownedInput_;
    std::unique_ptr<LuaDebugDraw> ownedDebugDraw_;

    ScriptChunk& loadChunk(const std::string& scriptName);
    sol::table instantiate(const ScriptChunk& chunk,
                           EntityHandle entity,
                           const std::unordered_map<std::string, nlohmann::json>& propertyValues);
    void callOnStart(ScriptInstance& inst);
    void reload(ScriptInstance& inst);
    void checkHotReload();
    void rebuildPackagePath();

    std::filesystem::path pathFor(const std::string& name) const {
        auto it = registeredScripts_.find(name);
        return it != registeredScripts_.end() ? it->second : std::filesystem::path{};
    }

    template<typename Fn>
    void safeCall(const char* context, Fn&& fn) {
        try {
            fn();
        } catch (const sol::error& e) {
            LOG4CXX_ERROR(logger_, "[" << context << "] Lua error: " << e.what());
        } catch (const std::exception& e) {
            LOG4CXX_ERROR(logger_, "[" << context << "] C++ exception: " << e.what());
        } catch (...) {
            LOG4CXX_ERROR(logger_, "[" << context << "] Unknown exception");
        }
    }
};
