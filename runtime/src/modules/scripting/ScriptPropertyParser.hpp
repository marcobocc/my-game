#pragma once
#include <filesystem>
#include <fstream>
#include <log4cxx/logger.h>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <vector>

struct ScriptPropertyDescriptor {
    std::string name;
    std::string type; // "float", "int", "bool", "string", "vec3", "color", "entity"
    nlohmann::json defaultValue;
};

// Parses the Properties table from a Lua script file without executing it.
// Reads the source as text and extracts property descriptors from the literal
// table definition — avoids spinning up a second sol::state (which crashes
// LuaJIT when the main Lua state is already alive on the same thread).
//
// Supports only the static literal syntax used by editor-authored scripts:
//   Properties = {
//     { name = "speed", type = "float", default = 1.0 },
//     { name = "target", type = "entity" },
//   }
class ScriptPropertyParser {
public:
    static std::vector<ScriptPropertyDescriptor> parse(const std::filesystem::path& scriptPath) {
        auto logger = log4cxx::Logger::getLogger("ScriptPropertyParser");

        std::ifstream file(scriptPath);
        if (!file.is_open()) {
            LOG4CXX_WARN(logger, "Cannot open script for property parsing: " << scriptPath);
            return {};
        }
        std::string src((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

        // Find "Properties" assignment
        auto propsPos = src.find("Properties");
        if (propsPos == std::string::npos) return {};

        // Find the opening '{' of the outer table
        auto outerOpen = src.find('{', propsPos);
        if (outerOpen == std::string::npos) return {};

        // Extract the balanced outer table text
        auto outerSrc = extractBalanced(src, outerOpen);
        if (outerSrc.empty()) return {};

        // Walk inner entries: each entry is a balanced { } block
        std::vector<ScriptPropertyDescriptor> result;
        std::size_t pos = 0;
        while (pos < outerSrc.size()) {
            auto entryOpen = outerSrc.find('{', pos);
            if (entryOpen == std::string::npos) break;
            auto entrySrc = extractBalanced(outerSrc, entryOpen);
            if (!entrySrc.empty()) {
                auto desc = parseEntry(entrySrc, logger);
                if (desc) result.push_back(std::move(*desc));
            }
            pos = entryOpen + 1;
        }
        return result;
    }

private:
    // Returns the content between the matching braces starting at `openPos`
    // (not including the braces themselves), or empty string on failure.
    static std::string extractBalanced(const std::string& src, std::size_t openPos) {
        int depth = 0;
        std::size_t start = openPos + 1;
        for (std::size_t i = openPos; i < src.size(); ++i) {
            if (src[i] == '{')
                ++depth;
            else if (src[i] == '}') {
                --depth;
                if (depth == 0) return src.substr(start, i - start);
            }
        }
        return {};
    }

    // Extracts a string value for a key like:  name = "foo"
    static std::optional<std::string> extractString(const std::string& src, const std::string& key) {
        auto pos = src.find(key);
        while (pos != std::string::npos) {
            auto eq = src.find('=', pos + key.size());
            if (eq == std::string::npos) break;
            auto q1 = src.find('"', eq + 1);
            if (q1 == std::string::npos) break;
            auto q2 = src.find('"', q1 + 1);
            if (q2 == std::string::npos) break;
            return src.substr(q1 + 1, q2 - q1 - 1);
        }
        return std::nullopt;
    }

    // Extracts a numeric/bool/nil token value for a key like:  default = 1.5
    static std::optional<std::string> extractToken(const std::string& src, const std::string& key) {
        auto pos = src.find(key);
        while (pos != std::string::npos) {
            auto eq = src.find('=', pos + key.size());
            if (eq == std::string::npos) break;
            // Skip whitespace after '='
            std::size_t start = eq + 1;
            while (start < src.size() && (src[start] == ' ' || src[start] == '\t'))
                ++start;
            if (start >= src.size()) break;
            // If it starts with '"' this is a string, not a token
            if (src[start] == '"') break;
            // Read until comma, newline, or closing brace
            std::size_t end = start;
            while (end < src.size() && src[end] != ',' && src[end] != '\n' && src[end] != '}')
                ++end;
            std::string tok = src.substr(start, end - start);
            // Trim trailing whitespace
            while (!tok.empty() && (tok.back() == ' ' || tok.back() == '\t' || tok.back() == '\r'))
                tok.pop_back();
            if (!tok.empty()) return tok;
            break;
        }
        return std::nullopt;
    }

    static std::optional<ScriptPropertyDescriptor> parseEntry(const std::string& src,
                                                              const log4cxx::LoggerPtr& logger) {
        auto name = extractString(src, "name");
        if (!name || name->empty()) return std::nullopt;

        auto type = extractString(src, "type");
        std::string typeStr = type.value_or("float");

        nlohmann::json defaultVal = defaultForType(typeStr, src, logger);
        return ScriptPropertyDescriptor{std::move(*name), std::move(typeStr), std::move(defaultVal)};
    }

    static nlohmann::json
    defaultForType(const std::string& type, const std::string& src, const log4cxx::LoggerPtr& logger) {
        if (type == "float" || type == "int") {
            if (auto tok = extractToken(src, "default")) {
                try {
                    return nlohmann::json(std::stod(*tok));
                } catch (...) {
                }
            }
            return nlohmann::json(0.0);
        }
        if (type == "bool") {
            if (auto tok = extractToken(src, "default")) {
                if (*tok == "true") return nlohmann::json(true);
                if (*tok == "false") return nlohmann::json(false);
            }
            return nlohmann::json(false);
        }
        if (type == "string") {
            if (auto val = extractString(src, "default")) return nlohmann::json(*val);
            return nlohmann::json("");
        }
        if (type == "vec3" || type == "color") {
            // Look for default = { n, n, n } or default = { n, n, n, n }
            auto defPos = src.find("default");
            if (defPos != std::string::npos) {
                auto eq = src.find('=', defPos + 7);
                if (eq != std::string::npos) {
                    auto brace = src.find('{', eq + 1);
                    if (brace != std::string::npos) {
                        auto inner = extractBalanced(src, brace);
                        auto nums = parseNumberList(inner);
                        if (!nums.empty()) {
                            auto arr = nlohmann::json::array();
                            for (double n: nums)
                                arr.push_back(n);
                            return arr;
                        }
                    }
                }
            }
            return type == "color" ? nlohmann::json::array({1.0, 1.0, 1.0, 1.0})
                                   : nlohmann::json::array({0.0, 0.0, 0.0});
        }
        if (type == "entity") return nlohmann::json(static_cast<uint64_t>(0));

        LOG4CXX_WARN(logger, "Unknown property type: " << type << " — defaulting to float");
        return nlohmann::json(0.0);
    }

    static std::vector<double> parseNumberList(const std::string& src) {
        std::vector<double> nums;
        std::size_t pos = 0;
        while (pos < src.size()) {
            while (pos < src.size() &&
                   (src[pos] == ' ' || src[pos] == '\t' || src[pos] == '\n' || src[pos] == '\r' || src[pos] == ','))
                ++pos;
            if (pos >= src.size()) break;
            // Try to parse a number starting here
            char* end = nullptr;
            double val = std::strtod(src.c_str() + pos, &end);
            if (end == src.c_str() + pos) break; // no number found
            nums.push_back(val);
            pos = static_cast<std::size_t>(end - src.c_str());
        }
        return nums;
    }
};
