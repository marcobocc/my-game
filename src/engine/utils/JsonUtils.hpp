#pragma once
#include <filesystem>
#include <fstream>
#include <glm/vec4.hpp>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>

namespace JsonUtils {
    inline nlohmann::json loadJson(const std::filesystem::path& path) {
        try {
            std::ifstream f(path);
            if (!f) {
                throw std::runtime_error("Could not open file: " + path.string());
            }
            nlohmann::json j;
            f >> j;
            return j;
        } catch (const std::exception& e) {
            throw std::runtime_error(std::string("Failed to load json: ") + e.what());
        }
    }

    template<typename T>
    T getRequired(const nlohmann::json& j, const char* key) {
        if (!j.contains(key)) {
            throw std::runtime_error(std::string("Missing field: ") + key);
        }
        try {
            return j.at(key).get<T>();
        } catch (...) {
            throw std::runtime_error(std::string("Invalid type for field: ") + key);
        }
    }

    template<typename T>
    T getOptional(const nlohmann::json& j, const char* key, T defaultValue) {
        if (!j.contains(key)) {
            return defaultValue;
        }
        try {
            return j.at(key).get<T>();
        } catch (...) {
            throw std::runtime_error(std::string("Invalid type for optional field: ") + key);
        }
    }
    template<>
    inline glm::vec4 getOptional(const nlohmann::json& j, const char* key, glm::vec4 defaultValue) {
        if (!j.contains(key)) return defaultValue;
        try {
            auto arr = j.at(key).get<std::array<float, 4>>();
            return glm::vec4(arr[0], arr[1], arr[2], arr[3]);
        } catch (...) {
            throw std::runtime_error(std::string("Invalid type for optional field: ") + key);
        }
    }
} // namespace JsonUtils
