#version 450

layout(location = 0) in vec3 inBaseColor;
layout(location = 1) in vec2 inUV;

layout(location = 0) out vec4 outAlbedo;

layout(set = 1, binding = 0) uniform sampler2D texSampler;

void main() {
    outAlbedo = vec4(inBaseColor, 1.0) * texture(texSampler, inUV);
}
