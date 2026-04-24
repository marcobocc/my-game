#version 450

layout(location = 0) out uint outObjectId;

layout(push_constant) uniform PushConstants {
    mat4 model;
    uint objectId;
} pc;

void main() {
    outObjectId = pc.objectId;
}
