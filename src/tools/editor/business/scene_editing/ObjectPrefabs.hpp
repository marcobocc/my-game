#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <nlohmann/json.hpp>
#include "modules/assets/BuiltinAssetNames.hpp"
#include "structs/components/Renderer.hpp"
#include "structs/components/Transform.hpp"

namespace primitives {
    namespace details {
        inline nlohmann::json makePrimitive(const std::string& meshName, const std::string& objectName) {
            Transform t{glm::vec3(0.0f), glm::quat(1.0f, 0.0f, 0.0f, 0.0f), glm::vec3(1.0f)};
            Renderer r{meshName, "materials/_solid_color.mat", std::nullopt, true};
            return {
                    {"metadata", {{"name", objectName}}},
                    {"transform", t.serialize()},
                    {"renderer", r.serialize()},
            };
        }
    } // namespace details
    inline nlohmann::json cube() { return details::makePrimitive(PRIMITIVE_GEOMETRY_CUBE, "Cube"); }
    inline nlohmann::json plane() { return details::makePrimitive(PRIMITIVE_GEOMETRY_PLANE, "Plane"); }
    inline nlohmann::json sphere(uint32_t resolution) {
        return details::makePrimitive("_PRIMITIVE_SPHERE_" + std::to_string(resolution), "Sphere");
    }
} // namespace primitives
