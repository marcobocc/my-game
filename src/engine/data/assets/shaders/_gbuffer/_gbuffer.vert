#version 450

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;

layout(location = 0) out vec3 outTint;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec2 outUV;

layout(push_constant) uniform PushConstants {
    mat4 model;
    vec4 tint;
    vec2 tiling;
    vec2 offset;
    int scaleInvariantUV;
} pc;

layout(set = 0, binding = 0) uniform CameraUBO {
    mat4 view;
    mat4 proj;
} camera;

void main() {
    gl_Position = camera.proj * camera.view * pc.model * vec4(inPos, 1.0);
    outTint = pc.tint.rgb;
    vec3 scale = vec3(
        length(pc.model[0].xyz),
        length(pc.model[1].xyz),
        length(pc.model[2].xyz)
    );
    vec2 finalTiling = mix(pc.tiling, pc.tiling * scale.xz, pc.scaleInvariantUV);
    outUV = inUV * finalTiling + pc.offset;
    outNormal = mat3(pc.model) * inNormal;
}
