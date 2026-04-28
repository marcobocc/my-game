#version 450

layout(location = 0) in vec3 inBaseColor;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;

layout(location = 0) out vec4 outAlbedo;
layout(location = 1) out vec4 outNormal;

layout(set = 1, binding = 0) uniform sampler2D texSampler;

void main() {
    outAlbedo = vec4(inBaseColor, 1.0) * texture(texSampler, inUV);
    outNormal = vec4(normalize(inNormal) * 0.5 + 0.5, 1.0);
}
