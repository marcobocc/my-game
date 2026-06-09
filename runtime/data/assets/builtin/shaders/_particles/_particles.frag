#version 450

layout(location = 0) in  vec4 fragColor;
layout(location = 1) in  vec2 fragUV;
layout(location = 0) out vec4 outColor;

layout(set = 1, binding = 1) uniform sampler2D spriteTexture;

layout(push_constant) uniform PC {
    uint hasSprite;
} pc;

void main() {
    vec4 color = fragColor;

    if (pc.hasSprite != 0u) {
        color *= texture(spriteTexture, fragUV);
    } else {
        // Soft circular fade
        float d     = length(fragUV - vec2(0.5)) * 2.0;
        float alpha = color.a * (1.0 - smoothstep(0.7, 1.0, d));
        color.a = alpha;
    }

    outColor = color;
}
