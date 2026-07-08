#version 450

layout(location = 0) in vec3 inWorldPos;

layout(push_constant) uniform PushConstants {
    mat4 lightSpaceMatrix;
    mat4 model;
    vec3 lightPos;
    float lightRange;
    uint isPointLight;
} pc;

void main() {
    if (pc.isPointLight == 1u) {
        // Store linear distance [0,1] so the lighting pass can compare without a projection matrix.
        gl_FragDepth = length(inWorldPos - pc.lightPos) / max(pc.lightRange, 0.001);
    } else {
        // Must write gl_FragDepth explicitly on all paths once it's used anywhere in the shader.
        gl_FragDepth = gl_FragCoord.z;
    }
}
