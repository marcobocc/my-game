#version 450

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec2 inUV;
layout(location = 0) out vec3 color;
layout(location = 1) out vec2 fragUV;

layout(push_constant) uniform PushConstants {
    mat4 model;
    vec4 baseColor;
} pc;

layout(set = 0, binding = 0) uniform CameraUBO {
    mat4 view;
    mat4 proj;
} camera;

void main() {
    gl_Position = camera.proj * camera.view * pc.model * vec4(inPos, 1.0);
    color = pc.baseColor.rgb;
    fragUV = inUV;
}

