#version 450

#define particleSpeed 0.3
#define particleSpread 4.0
#define particleShape 0.37
#define particleSize 1.0
#define particleSystemHeight 10.0

layout(set = 0, binding = 0) uniform Matrices {
    mat4 model;
    mat4 view;
    mat4 proj;
} uboMatrices;

layout(set = 0, binding = 3) uniform TimeUBO {
    float time;
} uboTime;

layout(location = 0) in vec3 inPosition;   // quad vertex (local quad, centered around 0)
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inNormal;

// Per-instance: use xy as world emitter position, z as seed
layout(location = 5) in vec3 inInstancePos;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 fragWorldPos;
layout(location = 2) out vec3 fragWorldNormal;
layout(location = 3) out vec2 fragTexCoord;
layout(location = 4) out float fragLife;

float hash(float n) {
    return fract(sin(n) * 43758.5453);
}

vec3 randomDir(float seed) {
    // Create a pseudo-random, normalized direction per instance
    float a = hash(seed * 1.23) * 6.28318530718; // azimuth
    float z = hash(seed * 4.56) * 2.0 - 1.0;     // cos(theta) in [-1,1]
    float r = sqrt(max(0.0, 1.0 - z * z));
    return normalize(vec3(r * cos(a), z, r * sin(a)));
}

void main()
{
    // Treat instance xy as emitter world position, z as stable seed
    vec3 emitterWorldPos = inInstancePos;
    float seed = inInstancePos.z;

    // Continuous burst: t in [0,1) loops; particles start at emitter and move outward
    float t = fract(seed + particleSpeed * uboTime.time);
    fragLife = t;

    // Direction per particle, with a small jitter to vary shape
    vec3 dir = randomDir(seed);
    float jitter = (hash(seed * 7.89) - 0.5) * particleShape;
    dir += vec3(jitter, jitter * 0.5, jitter);
    dir = normalize(dir);

    // Radial burst distance over lifetime
    float distance = particleSpread * t;

    // Optional vertical shaping: accelerate upward in first half, then decay
    float yLift = mix(particleSystemHeight * 0.5, -particleSystemHeight * 0.25, t);

    // Base particle position in world space: emitter + radial + vertical shaping
    vec3 baseWorldPos = emitterWorldPos + dir * distance + vec3(0.0, yLift, 0.0);

    // Quad offset in model/world space (not camera): use model X/Y as right/up
    vec3 modelRight = normalize(vec3(uboMatrices.model[0][0], uboMatrices.model[1][0], uboMatrices.model[2][0]));
    vec3 modelUp    = normalize(vec3(uboMatrices.model[0][1], uboMatrices.model[1][1], uboMatrices.model[2][1]));
    vec2 quadUV = inPosition.xy;
    vec3 billboardOffset = (quadUV.x * modelRight + quadUV.y * modelUp) * (particleSize * 0.5);

    // Compose final world position
    vec3 worldPos = baseWorldPos + billboardOffset;

    fragWorldPos = worldPos;
    fragWorldNormal = normalize(mat3(transpose(inverse(uboMatrices.model))) * inNormal);
    fragTexCoord = inTexCoord;
    fragColor = inColor;

    gl_Position = uboMatrices.proj * uboMatrices.view * vec4(worldPos, 1.0);
}