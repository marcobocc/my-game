#version 450

layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 outColor;

layout(push_constant) uniform Push {
    mat4 invViewProj;
    vec3 cameraPos;
    mat4 viewProj;
} pc;

vec3 getWorldPos(vec2 uv) {
    vec4 clip = vec4(uv * 2.0 - 1.0, 0.0, 1.0);
    vec4 world = pc.invViewProj * clip;
    world /= world.w;
    return world.xyz;
}

void main() {
    vec3 worldPos = getWorldPos(uv);
    vec3 dir = normalize(worldPos - pc.cameraPos);

    float t = -pc.cameraPos.y / dir.y;
    if (t < 0.0) discard;

    vec3 hit = pc.cameraPos + dir * t;

    vec4 clipHit = pc.viewProj * vec4(hit, 1.0);
    gl_FragDepth = clipHit.z / clipHit.w;

    float scale = 1.0;
    vec2 grid = hit.xz / scale;

    vec2 lines = abs(fract(grid - 0.5) - 0.5) / fwidth(grid);
    float line = min(lines.x, lines.y);

    float intensity = 1.0 - clamp(line, 0.0, 1.0);

    float axisWidth = 1.5;
    float axisX = 1.0 - clamp(abs(hit.x) / (fwidth(hit.x) * axisWidth), 0.0, 1.0);
    float axisZ = 1.0 - clamp(abs(hit.z) / (fwidth(hit.z) * axisWidth), 0.0, 1.0);

    vec3 color = vec3(0.2) * intensity;
    color = mix(color, vec3(1, 0, 0), axisX);  // X axis = red
    color = mix(color, vec3(0, 0, 1), axisZ);  // Z axis = blue

    outColor = vec4(color, intensity * 0.5);
}
