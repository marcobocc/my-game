#include "SceneSerializer.hpp"
#include <nlohmann/json.hpp>
#include "data/components/BoxCollider.hpp"
#include "data/components/Camera.hpp"
#include "data/components/Renderer.hpp"
#include "data/components/Transform.hpp"

using json = nlohmann::json;

// ---------------------------------------------------------------------------
// GLM helpers
// ---------------------------------------------------------------------------

static json vec3ToJson(const glm::vec3& v) { return {v.x, v.y, v.z}; }

static glm::vec3 vec3FromJson(const json& j) {
    auto a = j.get<std::array<float, 3>>();
    return {a[0], a[1], a[2]};
}

static json quatToJson(const glm::quat& q) {
    // stored as [x, y, z, w] — explicit to avoid GLM constructor order confusion
    return {q.x, q.y, q.z, q.w};
}

static glm::quat quatFromJson(const json& j) {
    auto a = j.get<std::array<float, 4>>();
    return glm::quat(a[3], a[0], a[1], a[2]); // glm::quat(w, x, y, z)
}

// ---------------------------------------------------------------------------
// Component serialization
// ---------------------------------------------------------------------------

static json serializeTransform(const Transform& t) {
    return {{"position", vec3ToJson(t.position)}, {"rotation", quatToJson(t.rotation)}, {"scale", vec3ToJson(t.scale)}};
}

static Transform deserializeTransform(const json& j) {
    Transform t{};
    t.position = vec3FromJson(j.at("position"));
    t.rotation = quatFromJson(j.at("rotation"));
    t.scale = vec3FromJson(j.at("scale"));
    return t;
}

static json serializeCamera(const Camera& c) {
    return {{"fov", c.fov}, {"aspect", c.aspect}, {"nearPlane", c.nearPlane}, {"farPlane", c.farPlane}};
}

static Camera deserializeCamera(const json& j) {
    Camera c{};
    c.fov = j.at("fov").get<float>();
    c.aspect = j.at("aspect").get<float>();
    c.nearPlane = j.at("nearPlane").get<float>();
    c.farPlane = j.at("farPlane").get<float>();
    return c;
}

static json serializeRenderer(const Renderer& r) {
    json j = {{"meshName", r.meshName}, {"materialName", r.materialName}, {"enabled", r.enabled}};
    if (r.baseColorOverride.has_value()) {
        const auto& c = *r.baseColorOverride;
        j["baseColorOverride"] = {c.r, c.g, c.b, c.a};
    }
    return j;
}

static Renderer deserializeRenderer(const json& j) {
    Renderer r{};
    r.meshName = j.at("meshName").get<std::string>();
    r.materialName = j.at("materialName").get<std::string>();
    r.enabled = j.at("enabled").get<bool>();
    if (j.contains("baseColorOverride")) {
        auto a = j.at("baseColorOverride").get<std::array<float, 4>>();
        r.baseColorOverride = glm::vec4(a[0], a[1], a[2], a[3]);
    }
    return r;
}

static json serializeBoxCollider(const BoxCollider& b) {
    return {{"center", vec3ToJson(b.center)}, {"halfExtents", vec3ToJson(b.halfExtents)}};
}

static BoxCollider deserializeBoxCollider(const json& j) {
    BoxCollider b{};
    b.center = vec3FromJson(j.at("center"));
    b.halfExtents = vec3FromJson(j.at("halfExtents"));
    return b;
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

namespace SceneSerializer {

    json serializeScene(const Scene& scene) {
        json root;
        json& objects = root["objects"];
        for (const auto& [name, obj]: scene.getObjects()) {
            json entry;
            if (obj.has<Transform>()) entry["transform"] = serializeTransform(obj.get<Transform>());
            if (obj.has<Camera>()) entry["camera"] = serializeCamera(obj.get<Camera>());
            if (obj.has<Renderer>()) entry["renderer"] = serializeRenderer(obj.get<Renderer>());
            if (obj.has<BoxCollider>()) entry["boxCollider"] = serializeBoxCollider(obj.get<BoxCollider>());
            objects[name] = std::move(entry);
        }

        return root;
    }

    void deserializeScene(const json& root, Scene& scene) {
        scene.clear();
        for (const auto& [name, entry]: root.at("objects").items()) {
            scene.createEmptyObject(name);
            auto& obj = scene.getObject(name);
            if (entry.contains("transform")) obj.add<Transform>() = deserializeTransform(entry["transform"]);
            if (entry.contains("camera")) obj.add<Camera>() = deserializeCamera(entry["camera"]);
            if (entry.contains("renderer")) obj.add<Renderer>() = deserializeRenderer(entry["renderer"]);
            if (entry.contains("boxCollider")) obj.add<BoxCollider>() = deserializeBoxCollider(entry["boxCollider"]);
        }
    }

} // namespace SceneSerializer
