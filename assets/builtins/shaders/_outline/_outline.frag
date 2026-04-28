#version 450

layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform usampler2D idBuffer;

layout(push_constant) uniform Push {
    vec2 texelSize;
    vec2 uvOffset;
    vec2 uvScale;
} pc;

void main() {
    // Remap screen UV [0,1] into the sub-region of the object ID texture that was rendered into
    vec2 mappedUV = pc.uvOffset + uv * pc.uvScale;

    uint centerID = texture(idBuffer, mappedUV).r;

    if (centerID == 0u) {
        discard;
    }

    const float radius = 5.0;
    bool isEdge = false;
    for (float x = -radius; x <= radius; x++) {
        for (float y = -radius; y <= radius; y++) {
            if (x == 0.0 && y == 0.0) continue;
            vec2 offset = vec2(x, y) * pc.texelSize;
            if (texture(idBuffer, mappedUV + offset).r != centerID) {
                isEdge = true;
                break;
            }
        }
        if (isEdge) break;
    }

    if (!isEdge) {
        discard;
    }

    outColor = vec4(1.0, 0.5, 0.0, 1.0);
}
