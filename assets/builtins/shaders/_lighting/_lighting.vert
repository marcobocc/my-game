#version 450

layout(location = 0) out vec2 outUV;

layout(push_constant) uniform Push {
    vec2 uvOffset;
    vec2 uvScale;
} pc;

void main() {
    // Fullscreen triangle — no vertex buffer needed.
    vec2 uv = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
    gl_Position = vec4(uv * 2.0 - 1.0, 0.0, 1.0);
    // Viewport is Y-flipped, so rasterized uv.y runs bottom→top.
    // Flip uvScale.y so we sample the gbuffer top→bottom as written.
    outUV = pc.uvOffset + vec2(uv.x, 1.0 - uv.y) * pc.uvScale;
}
