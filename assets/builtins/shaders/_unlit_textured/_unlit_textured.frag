#version 450

layout(location = 0) in vec3 color;
layout(location = 1) in vec2 fragUV;
layout(location = 0) out vec4 outColor;

layout(set = 1, binding = 0) uniform sampler2D texSampler;

void main() {
    vec4 texColor = texture(texSampler, fragUV);
    outColor = vec4(color, 1.0) * texColor;
}

