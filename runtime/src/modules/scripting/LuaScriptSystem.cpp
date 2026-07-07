#include "LuaScriptSystem.hpp"
#include <glm/glm.hpp>
#include <log4cxx/logger.h>
#include "LuaBindings.hpp"
#include "ScriptPropertyParser.hpp"
#include "modules/scene/components/BehaviourScript.hpp"

void LuaScriptSystem::init(World& world,
                           InputSystem& input,
                           DebugDraw& debugDraw,
                           const std::filesystem::path& scriptsDir) {
    logger_ = log4cxx::Logger::getLogger("LuaScriptSystem");
    world_ = &world;
    scriptsDir_ = scriptsDir;

    lua_.open_libraries(sol::lib::base,
                        sol::lib::math,
                        sol::lib::string,
                        sol::lib::table,
                        sol::lib::io,
                        sol::lib::package,
                        sol::lib::debug);

    lua_.set_exception_handler([](lua_State* L, sol::optional<const std::exception&> e, sol::string_view) -> int {
        if (e)
            luaL_error(L, "C++ exception: %s", e->what());
        else
            luaL_error(L, "Unknown C++ exception");
        return 0;
    });

    // Point require() at the scripts directory
    std::string packagePath = (scriptsDir_ / "?.lua").string();
    lua_["package"]["path"] = packagePath;

    ownedWorld_ = std::make_unique<LuaWorld>(world);
    ownedInput_ = std::make_unique<LuaInput>(input);
    ownedDebugDraw_ = std::make_unique<LuaDebugDraw>(debugDraw);
    worldFacade_ = ownedWorld_.get();
    inputFacade_ = ownedInput_.get();
    debugDrawFacade_ = ownedDebugDraw_.get();

    worldFacade_->setScriptLookup(
            [this](EntityHandle entity, const std::string& scriptName) -> std::optional<sol::table> {
                for (auto& inst: instances_)
                    if (inst.entity == entity && inst.scriptName == scriptName) return inst.self;
                return std::nullopt;
            });

    LuaBindings::registerAll(lua_, *worldFacade_, *inputFacade_, *debugDrawFacade_);

    // Instantiate one ScriptInstance per BehaviourScript component (entities may have multiple)
    for (const auto& actor: world.getActors()) {
        for (const auto* script: actor->getComponents<BehaviourScript>()) {
            if (script->scriptName.empty()) continue;
            safeCall(script->scriptName.c_str(), [&] {
                ScriptChunk& chunk = loadChunk(script->scriptName);
                sol::table self = instantiate(chunk, actor->handle(), script->propertyValues);
                instances_.push_back({actor->handle(), script->scriptName, std::move(self)});
                callOnStart(instances_.back());
            });
        }
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

    // Record which properties are declared as type "entity" so their injected
    // handle values get wrapped into Entity objects rather than left as integers.
    std::unordered_set<std::string> entityProps;
    for (const auto& desc: ScriptPropertyParser::parse(path))
        if (desc.type == "entity") entityProps.insert(desc.name);

    chunks_[scriptName] = {std::move(proto), writeTime, std::move(entityProps)};
    return chunks_[scriptName];
}

sol::table LuaScriptSystem::instantiate(const ScriptChunk& chunk,
                                        EntityHandle entity,
                                        const std::unordered_map<std::string, nlohmann::json>& propertyValues) {
    sol::table self = lua_.create_table();
    for (auto& [k, v]: chunk.proto)
        self[k] = v;
    self["entity"] = worldFacade_->wrap(entity);

    // Inject editor-set property values into self before onStart runs
    for (const auto& [key, val]: propertyValues) {
        if (chunk.entityProps.count(key)) {
            // Entity-typed property: wrap the stored handle into an Entity.
            self[key] = worldFacade_->wrap(val.is_number_integer() ? static_cast<EntityHandle>(val.get<int64_t>())
                                                                   : INVALID_ENTITY_HANDLE);
        } else if (val.is_number_integer()) {
            self[key] = static_cast<lua_Integer>(val.get<int64_t>());
        } else if (val.is_number()) {
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

void LuaScriptSystem::callOnCollision(EntityHandle self, EntityHandle other) {
    for (auto& inst: instances_) {
        if (inst.entity != self) continue;
        sol::protected_function onCollision = inst.self["onCollision"];
        if (!onCollision.valid()) continue;
        safeCall(inst.scriptName.c_str(), [&] {
            auto result = onCollision(inst.self, worldFacade_->wrap(other));
            if (!result.valid()) {
                sol::error err = result;
                LOG4CXX_ERROR(logger_, "[" << inst.scriptName << "] onCollision error: " << err.what());
            }
        });
    }
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
        inst.self = instantiate(chunk, inst.entity, propertyValues);
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
