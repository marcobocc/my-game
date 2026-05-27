#version 450

layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform usampler2D idBuffer;

layout(push_constant) uniform Push {
    vec2 texelSize;
    vec2 uvOffset;
    vec2 uvScale;
    uint targetCount;
    uint targetIds[16];
} pc;

bool isSelected(uint id) {
    for (uint i = 0; i < pc.targetCount; i++) {
        if (id == pc.targetIds[i]) return true;
    }
    return false;
}

void main() {
    vec2 mappedUV = pc.uvOffset + uv * pc.uvScale;

    uint centerID = texture(idBuffer, mappedUV).r;

    if (isSelected(centerID)) {
        discard;
    }

    const int radius = 4;
    bool nearTarget = false;
    for (int r = 1; r <= radius; r++) {
        float fr = float(r);
        vec2 offsets[4] = vec2[](
            vec2( fr,  0.0) * pc.texelSize,
            vec2(-fr,  0.0) * pc.texelSize,
            vec2(0.0,  fr)  * pc.texelSize,
            vec2(0.0, -fr)  * pc.texelSize
        );
        for (int i = 0; i < 4; i++) {
            if (isSelected(texture(idBuffer, mappedUV + offsets[i]).r)) {
                nearTarget = true;
                break;
            }
        }
        if (nearTarget) break;
    }

    if (!nearTarget) {
        discard;
    }

    outColor = vec4(1.0, 0.5, 0.0, 1.0);
}
