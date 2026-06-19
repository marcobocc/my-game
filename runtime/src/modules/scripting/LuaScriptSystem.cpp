#include "LuaScriptSystem.hpp"
#include <glm/glm.hpp>
#include <log4cxx/logger.h>
#include "modules/scene/components/BehaviourScript.hpp"

void LuaScriptSystem::init(World& world, InputSystem& input, const std::filesystem::path& scriptsDir) {
    logger_ = log4cxx::Logger::getLogger("LuaScriptSystem");
    world_ = &world;
    scriptsDir_ = scriptsDir;

    lua_.open_libraries(
            sol::lib::base, sol::lib::math, sol::lib::string, sol::lib::table, sol::lib::io, sol::lib::package);

    // Point require() at the scripts directory
    std::string packagePath = (scriptsDir_ / "?.lua").string();
    lua_["package"]["path"] = packagePath;

    ownedWorld_ = std::make_unique<LuaWorld>(world);
    ownedInput_ = std::make_unique<LuaInput>(input);
    worldFacade_ = ownedWorld_.get();
    inputFacade_ = ownedInput_.get();

    LuaBindings::registerAll(lua_, *worldFacade_, *inputFacade_);

    // Instantiate one ScriptInstance per BehaviourScript component
    for (const auto& [handle, script]: world.query<BehaviourScript>()) {
        if (script->scriptName.empty()) continue;
        safeCall(script->scriptName.c_str(), [&] {
            ScriptChunk& chunk = loadChunk(script->scriptName);
            sol::table self = instantiate(chunk.proto, handle, script->propertyValues);
            instances_.push_back({handle, script->scriptName, std::move(self)});
            callOnStart(instances_.back());
        });
    }

    LOG4CXX_INFO(logger_, "Initialized — " << instances_.size() << " script instance(s)");
}

void LuaScriptSystem::update(float dt) {
    checkHotReload();

    for (auto& inst: instances_) {
        sol::protected_function onUpdate = inst.self["onUpdate"];
        if (!onUpdate.valid()) continue;
        safeCall(inst.scriptName.c_str(), [&] {
            auto result = onUpdate(inst.self, dt);
            if (!result.valid()) {
                sol::error err = result;
                LOG4CXX_ERROR(logger_, "[" << inst.scriptName << "] onUpdate error: " << err.what());
            }
        });
    }
}

LuaScriptSystem::ScriptChunk& LuaScriptSystem::loadChunk(const std::string& scriptName) {
    auto it = chunks_.find(scriptName);
    if (it != chunks_.end()) return it->second;

    auto path = pathFor(scriptName);
    auto writeTime = std::filesystem::last_write_time(path);

    sol::protected_function_result result = lua_.safe_script_file(path.string());
    if (!result.valid()) {
        sol::error err = result;
        throw sol::error(err.what());
    }

    sol::table proto = result;
    chunks_[scriptName] = {std::move(proto), writeTime};
    return chunks_[scriptName];
}

sol::table LuaScriptSystem::instantiate(sol::table proto,
                                        EntityHandle entity,
                                        const std::unordered_map<std::string, nlohmann::json>& propertyValues) {
    sol::table self = lua_.create_table();
    for (auto& [k, v]: proto)
        self[k] = v;
    self["entity"] = entity;

    // Inject editor-set property values into self before onStart runs
    for (const auto& [key, val]: propertyValues) {
        if (val.is_number()) {
            self[key] = val.get<double>();
        } else if (val.is_boolean()) {
            self[key] = val.get<bool>();
        } else if (val.is_string()) {
            self[key] = val.get<std::string>();
        } else if (val.is_array() && val.size() == 3) {
            self[key] = glm::vec3{val[0].get<float>(), val[1].get<float>(), val[2].get<float>()};
        } else if (val.is_array() && val.size() == 4) {
            self[key] = glm::vec4{val[0].get<float>(), val[1].get<float>(), val[2].get<float>(), val[3].get<float>()};
        }
    }

    return self;
}

void LuaScriptSystem::callOnStart(ScriptInstance& inst) {
    sol::protected_function onStart = inst.self["onStart"];
    if (!onStart.valid()) return;
    safeCall(inst.scriptName.c_str(), [&] {
        auto result = onStart(inst.self);
        if (!result.valid()) {
            sol::error err = result;
            LOG4CXX_ERROR(logger_, "[" << inst.scriptName << "] onStart error: " << err.what());
        }
    });
}

void LuaScriptSystem::reload(ScriptInstance& inst) {
    LOG4CXX_INFO(logger_, "Hot-reloading: " << inst.scriptName);

    // Collect current propertyValues from the world so hot-reload preserves them
    std::unordered_map<std::string, nlohmann::json> propertyValues;
    if (world_) {
        if (Actor* actor = world_->getActor(inst.entity)) {
            for (BehaviourScript* bs: actor->getComponents<BehaviourScript>()) {
                if (bs->scriptName == inst.scriptName) {
                    propertyValues = bs->propertyValues;
                    break;
                }
            }
        }
    }

    // Fully release old state before reloading
    inst.self = sol::lua_nil;
    chunks_.erase(inst.scriptName);

    safeCall(inst.scriptName.c_str(), [&] {
        ScriptChunk& chunk = loadChunk(inst.scriptName);
        inst.self = instantiate(chunk.proto, inst.entity, propertyValues);
        callOnStart(inst);
    });
}

void LuaScriptSystem::checkHotReload() {
    for (auto& inst: instances_) {
        auto path = pathFor(inst.scriptName);
        std::error_code ec;
        auto t = std::filesystem::last_write_time(path, ec);
        if (ec) continue;

        auto it = chunks_.find(inst.scriptName);
        if (it != chunks_.end() && t != it->second.lastWriteTime) {
            it->second.lastWriteTime = t;
            reload(inst);
        }
    }
}
