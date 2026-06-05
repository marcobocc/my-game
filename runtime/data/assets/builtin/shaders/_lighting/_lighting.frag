#version 450

layout(location = 0) in vec2 inUV;

layout(location = 0) out vec4 outColor;

layout(push_constant) uniform Push {
    mat4 lightSpaceFromScreen; // lightSpaceMatrix * inverse(proj * view)
    mat4 invViewProj;          // inverse(proj * view) — reconstruct world pos for spotlights
    vec2 uvOffset;
    vec2 uvScale;
    uint enableLighting;
    uint _pad;
} pc;

// SSBO layout:
//   header[0].x = lightIndex    — which light this pass processes
//   header[0].y = isFirstLight  — 1 = first pass (write ambient + replace), 0 = additive
//   header[0].z = lightCount    — total lights in scene
//
// Each light occupies LIGHT_STRIDE vec4s in data[]:
//   [0] = (direction.xyz, type)        type: 0 = directional, 1 = spot, 2 = point
//   [1] = (color.rgb, intensity)
//   [2] = (position.xyz, unused)
//   [3] = (cosInnerCone, cosOuterCone, range, unused)
const uint LIGHT_STRIDE = 4u;
const float LIGHT_DIRECTIONAL = 0.0;
const float LIGHT_SPOT = 1.0;
const float LIGHT_POINT = 2.0;

layout(set = 0, binding = 0) uniform sampler2D albedoSampler;
layout(set = 0, binding = 1) uniform sampler2D normalSampler;
layout(set = 0, binding = 2) readonly buffer LightsSSBO {
    uvec4 header[2];
    vec4  data[];
} lightsBuffer;
layout(set = 0, binding = 3) uniform sampler2D shadowMap;
layout(set = 0, binding = 4) uniform sampler2D gbufferDepth;
layout(set = 0, binding = 5) uniform samplerCube shadowCube;

const vec3 AMBIENT = vec3(0.15);

float calculateShadow() {
    // Reconstruct NDC position from G-Buffer depth at this texel.
    float depth = texture(gbufferDepth, inUV).r;
    // Nothing written here (sky/background) — no shadow.
    if (depth >= 1.0) return 0.0;

    // inUV is sub-viewport UV: uvOffset + rawUV * uvScale.
    // NDC must be built from the raw fullscreen UV, not the sub-viewport UV.
    vec2 rawUV = (inUV - pc.uvOffset) / pc.uvScale;
    vec4 ndcPos = vec4(rawUV * 2.0 - 1.0, depth, 1.0);

    // Transform directly from NDC to light space using the precomputed combined matrix.
    vec4 lightSpacePos = pc.lightSpaceFromScreen * ndcPos;
    vec3 projCoords = lightSpacePos.xyz / lightSpacePos.w;
    // The light-space matrix already emits z in [0,1] (Vulkan depth range), so only the
    // XY coordinates need the [-1,1] -> [0,1] remap to become shadow-map UVs.
    projCoords.xy = projCoords.xy * 0.5 + 0.5;

    if (projCoords.z > 1.0 || projCoords.z < 0.0 || projCoords.x < 0.0 || projCoords.x > 1.0 ||
        projCoords.y < 0.0 || projCoords.y > 1.0)
        return 0.0;

    // Self-shadow acne is handled by slope-scaled hardware depth bias in the shadow pass,
    // so the comparison here is a plain depth test.
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    for (int x = -1; x <= 1; ++x) {
        for (int y = -1; y <= 1; ++y) {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += projCoords.z > pcfDepth ? 1.0 : 0.0;
        }
    }
    return shadow / 9.0;
}

// Point light shadow: sample the cubemap with the world-space vector from the light to the fragment.
// The cubemap stores linear distance normalised by range, so comparison is straightforward.
float calculatePointShadow(vec3 worldPos, vec3 lightPos, float range) {
    vec3 fragToLight = worldPos - lightPos;
    float currentDist = length(fragToLight) / max(range, 0.001);
    if (currentDist >= 1.0) return 0.0;

    // Simple single-sample comparison (PCF on cube maps is expensive; bias handles acne).
    float closestDist = texture(shadowCube, fragToLight).r;
    const float bias = 0.01;
    return currentDist - bias > closestDist ? 1.0 : 0.0;
}

vec3 reconstructWorldPos(float depth) {
    vec2 rawUV = (inUV - pc.uvOffset) / pc.uvScale;
    vec4 ndcPos = vec4(rawUV * 2.0 - 1.0, depth, 1.0);
    vec4 worldPos = pc.invViewProj * ndcPos;
    return worldPos.xyz / worldPos.w;
}

void main() {
    uint lightIndex   = lightsBuffer.header[0].x;
    uint isFirstLight = lightsBuffer.header[0].y;
    uint lightCount   = lightsBuffer.header[0].z;

    vec4 albedoSample = texture(albedoSampler, inUV);
    if (albedoSample.a == 0.0) {
        outColor = (isFirstLight == 1u) ? vec4(0.15, 0.15, 0.15, 1.0) : vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }

    if (pc.enableLighting == 0u) {
        outColor = (isFirstLight == 1u) ? vec4(albedoSample.rgb, 1.0) : vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }

    vec3 normal = normalize(texture(normalSampler, inUV).xyz * 2.0 - 1.0);
    vec3 lighting = (isFirstLight == 1u) ? AMBIENT : vec3(0.0);

    if (lightCount == 0u) {
        outColor = vec4(albedoSample.rgb * lighting, 1.0);
        return;
    }

    float surfaceDepth = texture(gbufferDepth, inUV).r;
    vec3 worldPos = reconstructWorldPos(surfaceDepth);

    uint base = lightIndex * LIGHT_STRIDE;
    vec4 dirType        = lightsBuffer.data[base + 0u];
    vec4 colorIntensity = lightsBuffer.data[base + 1u];
    vec4 posUnused      = lightsBuffer.data[base + 2u];
    vec4 cone           = lightsBuffer.data[base + 3u];

    float type      = dirType.w;
    vec3 lightColor = colorIntensity.rgb;
    float intensity = colorIntensity.w;
    vec3 lightPos   = posUnused.xyz;

    float diffuse = 0.0;
    float shadow  = 0.0;

    if (type == LIGHT_SPOT) {
        vec3 toLight  = lightPos - worldPos;
        float dist    = length(toLight);
        vec3 L        = toLight / max(dist, 1e-4);

        vec3  spotAxis   = normalize(-dirType.xyz);
        float cosTheta   = dot(-L, spotAxis);
        float cosInner   = cone.x;
        float cosOuter   = cone.y;
        float range      = cone.z;

        float coneFalloff = clamp((cosTheta - cosOuter) / max(cosInner - cosOuter, 1e-4), 0.0, 1.0);
        float distAtten   = clamp(1.0 - dist / max(range, 1e-4), 0.0, 1.0);
        distAtten *= distAtten;

        diffuse = max(dot(normal, L), 0.0) * intensity * coneFalloff * distAtten;
        shadow  = calculateShadow();
    } else if (type == LIGHT_POINT) {
        vec3 toLight = lightPos - worldPos;
        float dist   = length(toLight);
        vec3 L       = toLight / max(dist, 1e-4);
        float range  = cone.z;

        float distAtten = clamp(1.0 - dist / max(range, 1e-4), 0.0, 1.0);
        distAtten *= distAtten;

        diffuse = max(dot(normal, L), 0.0) * intensity * distAtten;
        shadow  = calculatePointShadow(worldPos, lightPos, range);
    } else {
        // Directional
        vec3 lightDir = normalize(dirType.xyz);
        diffuse = max(dot(normal, lightDir), 0.0) * intensity;
        shadow  = calculateShadow();
    }

    lighting += lightColor * vec3(diffuse) * (1.0 - shadow);

    outColor = vec4(albedoSample.rgb * lighting, 1.0);
}
