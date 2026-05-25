#version 450

layout(location = 0) in vec2 inUV;

layout(location = 0) out vec4 outColor;

layout(push_constant) uniform Push {
    vec2 uvOffset;
    vec2 uvScale;
    uint enableLighting;
    uint lightCount;
} pc;

layout(set = 0, binding = 0) uniform sampler2D albedoSampler;
layout(set = 0, binding = 1) uniform sampler2D normalSampler;
layout(set = 0, binding = 2) readonly buffer LightsSSBO {
    vec4 data[];
} lightsBuffer;

const vec3 AMBIENT = vec3(0.15);

void main() {
    vec4 albedoSample = texture(albedoSampler, inUV);
    if (albedoSample.a == 0.0) {
        outColor = vec4(0.15, 0.15, 0.15, 1.0);
        return;
    }

    if (pc.enableLighting == 0) {
        outColor = vec4(albedoSample.rgb, 1.0);
        return;
    }

    vec3 normal = normalize(texture(normalSampler, inUV).xyz * 2.0 - 1.0);
    vec3 lighting = AMBIENT;

    for (uint i = 0; i < pc.lightCount; ++i) {
        vec3 lightDir = normalize(lightsBuffer.data[i * 2].xyz);
        float intensity = lightsBuffer.data[i * 2].w;
        float diffuse = max(dot(normal, lightDir), 0.0) * intensity;
        lighting += vec3(diffuse);
    }

    vec3 color = albedoSample.rgb * lighting;
    outColor = vec4(color, 1.0);
}
