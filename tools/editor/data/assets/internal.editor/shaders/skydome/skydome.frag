#version 450

layout(location = 0) in vec2 inUV;
layout(location = 1) in vec3 inDir;

layout(location = 0) out vec4 outColor;

void main() {
    // inDir is the unit sphere surface direction; y is vertical (-1 bottom, +1 top)
    float t = clamp(normalize(inDir).y, -1.0, 1.0);

    // Horizon band: map [-0.1, 0.15] to [0, 1] for a sharp-ish transition
    float horizon = smoothstep(-0.1, 0.15, t);
    vec3 zenith  = vec3(0.40, 0.50, 0.82); // deep blue
    vec3 mid     = vec3(0.52, 0.76, 0.95); // pale sky blue
    vec3 horiz   = vec3(0.98, 0.72, 0.42); // peachy orange

    // Two-stop gradient: horizon → mid → zenith
    float upper  = smoothstep(0.0, 1.0, horizon);
    vec3  color  = mix(horiz, mid, clamp(upper * 2.0, 0.0, 1.0));
    color        = mix(color, zenith, clamp((upper - 0.5) * 2.0, 0.0, 1.0));

    // Ground and below
    float groundMask = 1.0 - smoothstep(-0.05, 0.05, t);
    vec3 groundColor = vec3(0.4);    // gray
    color = mix(color, groundColor, groundMask);

    outColor = vec4(color, 1.0);
}
