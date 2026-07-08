#version 450

struct GpuParticle {
    vec3  position;
    float lifetime;
    vec3  velocity;
    float maxLifetime;
    vec4  colorStart;
    vec4  colorEnd;
    float sizeStart;
    float sizeEnd;
    float rotation;
    float _pad;
};

layout(set = 0, binding = 0) uniform CameraUBO {
    mat4 view;
    mat4 proj;
} camera;

layout(set = 1, binding = 0) readonly buffer ParticleBuffer {
    GpuParticle particles[];
};

layout(set = 1, binding = 2) uniform EmitterConfig {
    vec3  emitterPos;
    float lifetimeMin;
    vec3  gravity;
    float lifetimeMax;
    float coneAngle;
    float coneRadius;
    float speedMin;
    float speedMax;
    float sizeStart;
    float sizeEnd;
    float _pad0;
    float _pad1;
    vec4  colorStartMin;
    vec4  colorStartMax;
    vec4  colorEndMin;
    vec4  colorEndMax;
    float rotationMin;
    float rotationMax;
    int   colorKeyCount;
    float _pad2;
    vec4  gradientColors[8];
    vec4  gradientTimes[8];
} config;

layout(location = 0) out vec4  fragColor;
layout(location = 1) out vec2  fragUV;

const vec2 CORNERS[6] = vec2[](
    vec2(-1.0, -1.0), vec2( 1.0, -1.0), vec2(-1.0,  1.0),
    vec2(-1.0,  1.0), vec2( 1.0, -1.0), vec2( 1.0,  1.0)
);
const vec2 UVS[6] = vec2[](
    vec2(0.0, 0.0), vec2(1.0, 0.0), vec2(0.0, 1.0),
    vec2(0.0, 1.0), vec2(1.0, 0.0), vec2(1.0, 1.0)
);

// Evaluate the color over lifetime gradient at normalized time t
vec4 sampleGradient(float t) {
    int n = clamp(config.colorKeyCount, 2, 8);
    // Find the two surrounding stops
    for (int i = 0; i < n - 1; ++i) {
        float t0 = config.gradientTimes[i].x;
        float t1 = config.gradientTimes[i + 1].x;
        if (t <= t1) {
            float f = (t1 > t0) ? clamp((t - t0) / (t1 - t0), 0.0, 1.0) : 0.0;
            return mix(config.gradientColors[i], config.gradientColors[i + 1], f);
        }
    }
    return config.gradientColors[n - 1];
}

void main() {
    GpuParticle p = particles[gl_InstanceIndex];

    if (p.lifetime < 0.0) {
        gl_Position = vec4(0.0, 0.0, -2.0, 1.0);
        fragColor = vec4(0.0);
        fragUV = vec2(0.0);
        return;
    }

    float t     = clamp(1.0 - (p.lifetime / p.maxLifetime), 0.0, 1.0);
    vec4  color = mix(p.colorStart, p.colorEnd, t) * sampleGradient(t);
    float size  = mix(p.sizeStart,  p.sizeEnd,  t);

    vec3 right = vec3(camera.view[0][0], camera.view[1][0], camera.view[2][0]);
    vec3 up    = vec3(camera.view[0][1], camera.view[1][1], camera.view[2][1]);

    // Rotate billboard corner by particle's initial rotation
    float cosR = cos(p.rotation);
    float sinR = sin(p.rotation);
    vec2  c    = CORNERS[gl_VertexIndex];
    vec2  rc   = vec2(cosR * c.x - sinR * c.y,
                      sinR * c.x + cosR * c.y);

    vec3 worldPos = p.position
                  + right * rc.x * size
                  + up    * rc.y * size;

    gl_Position = camera.proj * camera.view * vec4(worldPos, 1.0);
    fragColor   = color;
    fragUV      = UVS[gl_VertexIndex];
}
