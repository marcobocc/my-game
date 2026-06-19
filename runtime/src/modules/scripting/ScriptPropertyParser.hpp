#pragma once
#include <filesystem>
#include <log4cxx/logger.h>
#include <nlohmann/json.hpp>
#include <optional>
#include <sol/sol.hpp>
#include <string>
#include <vector>

struct ScriptPropertyDescriptor {
    std::string name;
    std::string type; // "float", "int", "bool", "string", "vec3", "color"
    nlohmann::json defaultValue;
};

// Parses the Properties table from a Lua script without running game logic.
// Spins up a minimal sol::state with no engine bindings — World/Input are nil.
class ScriptPropertyParser {
public:
    static std::vector<ScriptPropertyDescriptor> parse(const std::filesystem::path& scriptPath) {
        auto logger = log4cxx::Logger::getLogger("ScriptPropertyParser");

        sol::state lua;
        lua.open_libraries(sol::lib::base, sol::lib::table);

        // Safe-execute: if the script errors (e.g. calls World which is nil), we still
        // attempt to read Properties — but if the chunk itself throws, return empty.
        auto result = lua.safe_script_file(scriptPath.string(), sol::script_pass_on_error);
        if (!result.valid()) {
            // Script may reference globals that don't exist in this minimal state.
            // Try to read Properties anyway — it may have been set before the error.
            sol::error err = result;
            LOG4CXX_DEBUG(logger, "Parse-mode script error (non-fatal): " << err.what());
        }

        sol::object propsObj = lua["Properties"];
        if (!propsObj.valid() || propsObj.get_type() != sol::type::table) return {};

        sol::table props = propsObj;
        std::vector<ScriptPropertyDescriptor> descriptors;

        for (auto& [k, v]: props) {
            if (v.get_type() != sol::type::table) continue;
            sol::table entry = v;

            std::string name = entry.get_or<std::string>("name", "");
            std::string type = entry.get_or<std::string>("type", "float");
            if (name.empty()) continue;

            nlohmann::json defaultVal = defaultForType(type, entry, logger);
            descriptors.push_back({std::move(name), std::move(type), std::move(defaultVal)});
        }

        return descriptors;
    }

private:
    static nlohmann::json defaultForType(const std::string& type, sol::table& entry, const log4cxx::LoggerPtr& logger) {
        if (type == "float" || type == "int") {
            auto v = entry.get<std::optional<double>>("default");
            return v ? nlohmann::json(*v) : nlohmann::json(0.0);
        }
        if (type == "bool") {
            auto v = entry.get<std::optional<bool>>("default");
            return v ? nlohmann::json(*v) : nlohmann::json(false);
        }
        if (type == "string") {
            auto v = entry.get<std::optional<std::string>>("default");
            return v ? nlohmann::json(*v) : nlohmann::json("");
        }
        if (type == "vec3") {
            sol::object d = entry["default"];
            if (d.get_type() == sol::type::table) {
                sol::table t = d;
                return nlohmann::json::array({t.get_or(1, 0.0), t.get_or(2, 0.0), t.get_or(3, 0.0)});
            }
            return nlohmann::json::array({0.0, 0.0, 0.0});
        }
        if (type == "color") {
            sol::object d = entry["default"];
            if (d.get_type() == sol::type::table) {
                sol::table t = d;
                return nlohmann::json::array({t.get_or(1, 1.0), t.get_or(2, 1.0), t.get_or(3, 1.0), t.get_or(4, 1.0)});
            }
            return nlohmann::json::array({1.0, 1.0, 1.0, 1.0});
        }
        LOG4CXX_WARN(logger, "Unknown property type: " << type << " — defaulting to float");
        return nlohmann::json(0.0);
    }
};
