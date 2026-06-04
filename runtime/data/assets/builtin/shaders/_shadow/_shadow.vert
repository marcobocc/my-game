#version 450

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;

layout(push_constant) uniform PushConstants {
    mat4 lightSpaceMatrix;
    mat4 model;
} pc;

void main() {
    gl_Position = pc.lightSpaceMatrix * pc.model * vec4(inPos, 1.0);
}
