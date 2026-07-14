#version 450

layout(location = 0) in vec3 inTint;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;

layout(location = 0) out vec4 outAlbedo;
layout(location = 1) out vec4 outNormal;

layout(set = 1, binding = 0) uniform sampler2D splatMap;
layout(set = 1, binding = 1) uniform sampler2D layerAlbedo0;
layout(set = 1, binding = 2) uniform sampler2D layerAlbedo1;
layout(set = 1, binding = 3) uniform sampler2D layerAlbedo2;
layout(set = 1, binding = 4) uniform sampler2D layerAlbedo3;

// Per-layer parameters taken from each layer's submaterial.
// tilingOffset = (tiling.x, tiling.y, offset.x, offset.y)
layout(set = 1, binding = 5) uniform LayerParams {
    vec4 tint[4];
    vec4 tilingOffset[4];
} layers;

vec3 sampleLayer(sampler2D tex, int i) {
    vec2 uv = inUV * layers.tilingOffset[i].xy + layers.tilingOffset[i].zw;
    return texture(tex, uv).rgb * layers.tint[i].rgb;
}

void main() {
    vec4 weights = texture(splatMap, inUV);

    vec3 blended = sampleLayer(layerAlbedo0, 0) * weights.r +
                   sampleLayer(layerAlbedo1, 1) * weights.g +
                   sampleLayer(layerAlbedo2, 2) * weights.b +
                   sampleLayer(layerAlbedo3, 3) * weights.a;

    outAlbedo = vec4(inTint, 1.0) * vec4(blended, 1.0);
    outNormal = vec4(normalize(inNormal) * 0.5 + 0.5, 1.0);
}
