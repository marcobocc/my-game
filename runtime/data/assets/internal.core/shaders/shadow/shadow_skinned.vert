#version 450

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;
layout(location = 3) in ivec4 inBoneIndices;
layout(location = 4) in vec4 inBoneWeights;

layout(push_constant) uniform PushConstants {
    mat4 lightSpaceMatrix;
    mat4 model;
    vec3 lightPos;
    float lightRange;
    uint isPointLight;
} pc;

layout(set = 0, binding = 0) uniform SkinningUBO {
    mat4 bones[256];
} skin;

layout(location = 0) out vec3 outWorldPos;

void main() {
    mat4 skinMatrix =
        inBoneWeights.x * skin.bones[inBoneIndices.x] +
        inBoneWeights.y * skin.bones[inBoneIndices.y] +
        inBoneWeights.z * skin.bones[inBoneIndices.z] +
        inBoneWeights.w * skin.bones[inBoneIndices.w];

    vec4 worldPos = pc.model * skinMatrix * vec4(inPos, 1.0);
    outWorldPos = worldPos.xyz;
    gl_Position = pc.lightSpaceMatrix * worldPos;
}
