#version 450

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;

layout(location = 0) out vec2 outUV;
layout(location = 1) out vec3 outDir;

layout(push_constant) uniform Push {
    mat4 viewProj;
    vec3 cameraPos;
    float _pad;
} pc;

void main() {
    vec3 worldPos = inPos + pc.cameraPos;
    vec4 clip = pc.viewProj * vec4(worldPos, 1.0);
    // Force depth to far plane so sky is always behind real geometry
    gl_Position = clip.xyww;
    outUV  = inUV;
    outDir = inPos;
}
