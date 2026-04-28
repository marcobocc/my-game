#version 450

layout(location = 0) in vec2 inUV;

layout(location = 0) out vec4 outColor;

layout(push_constant) uniform Push {
    vec2 uvOffset;
    vec2 uvScale;
    uint enableLighting;
} pc;

layout(set = 0, binding = 0) uniform sampler2D albedoSampler;
layout(set = 0, binding = 1) uniform sampler2D normalSampler;

const vec3 LIGHT_DIR = normalize(vec3(0.8, 1.0, 0.6));
const vec3 AMBIENT = vec3(0.15);

void main() {
    vec4 albedoSample = texture(albedoSampler, inUV);
    if (albedoSample.a == 0.0) {
        outColor = vec4(0.1, 0.1, 0.1, 1.0);
        return;
    }

    if (pc.enableLighting == 0) {
        outColor = vec4(albedoSample.rgb, 1.0);
        return;
    }

    vec3 normal = normalize(texture(normalSampler, inUV).xyz * 2.0 - 1.0);
    float diffuse = max(dot(normal, LIGHT_DIR), 0.0);
    vec3 color = albedoSample.rgb * (AMBIENT + diffuse);

    outColor = vec4(color, 1.0);
}
