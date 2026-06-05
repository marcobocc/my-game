#version 450

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;

layout(push_constant) uniform PushConstants {
    mat4 lightSpaceMatrix;
    mat4 model;
    vec3 lightPos;
    float lightRange;
    uint isPointLight;
} pc;

layout(location = 0) out vec3 outWorldPos;

void main() {
    vec4 worldPos = pc.model * vec4(inPos, 1.0);
    outWorldPos = worldPos.xyz;
    gl_Position = pc.lightSpaceMatrix * worldPos;
}
