#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <nlohmann/json.hpp>
#include "data/components/Renderer.hpp"
#include "data/components/Transform.hpp"
#include "modules/assets/BuiltinAssetNames.hpp"

namespace primitives {
    namespace details {
        inline nlohmann::json makePrimitive(const std::string& meshName, const std::string& objectName) {
            Transform t{glm::vec3(0.0f), glm::quat(1.0f, 0.0f, 0.0f, 0.0f), glm::vec3(1.0f)};
            Renderer r{meshName, "builtins/materials/_solid_color.mat", std::nullopt, true};
            return {
                    {"metadata", {{"name", objectName}}},
                    {"transform", t.serialize()},
                    {"renderer", r.serialize()},
            };
        }
    } // namespace details
    inline nlohmann::json cube() { return details::makePrimitive(PRIMITIVE_GEOMETRY_CUBE, "Cube"); }
    inline nlohmann::json plane() { return details::makePrimitive(PRIMITIVE_GEOMETRY_PLANE, "Plane"); }
    inline nlohmann::json sphere() { return details::makePrimitive("_PRIMITIVE_SPHERE_6", "Sphere"); }
} // namespace primitives
