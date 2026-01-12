#version 450

const int MAX_LIGHTS = 8;

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

struct GPULight {
    uint  type;      // 0=Point, 1=Directional, 2=Spot
    float ambient;
    float specular;
    float pad0;

    vec3  position;  float pad1;
    vec3  direction; float pad2;
    vec3  color;     float pad3;

    float innerCos;
    float outerCos;
    float range;
    float pad4;

    float attConst;
    float attLinear;
    float attQuadratic;
    float pad5;
};

layout(set = 0, binding = 2) uniform LightingUBO {
    vec3  viewPosWorld; float shininess;
    int   lightCount;   uint padA; uint padB; uint padC;
    GPULight lights[MAX_LIGHTS];
} lighting;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inNormal;

layout(location = 0) out vec3 fragColor;     // interpolated lighting result
layout(location = 1) out vec2 fragTexCoord;  // interpolated UVs
layout(location = 2) out vec3 fragNormal;    // world-space normal
layout(location = 3) out vec3 fragWorldPos;  // world-space position

float computeAttenuation(float dist, float k0, float k1, float k2) {
    return 1.0 / max(k0 + k1 * dist + k2 * dist * dist, 1e-5);
}

float spotFactor(vec3 Ldir, vec3 spotDir, float innerCos, float outerCos) {
    float c = dot(normalize(-Ldir), normalize(spotDir));
    return smoothstep(outerCos, innerCos, c);
}

void main() {
    vec4 worldPos = ubo.model * vec4(inPosition, 1.0);
    gl_Position = ubo.proj * ubo.view * worldPos;

    mat3 normalMatrix = transpose(inverse(mat3(ubo.model)));
    vec3 N = normalize(normalMatrix * inNormal);
    vec3 V = normalize(lighting.viewPosWorld - worldPos.xyz);

    vec3 accum = vec3(0.0);
    int count = clamp(lighting.lightCount, 0, MAX_LIGHTS);

    for (int i = 0; i < count; ++i) {
        GPULight L = lighting.lights[i];

        // Base ambient from vertex color
        vec3 ambientTerm = L.ambient * inColor;

        vec3 Ldir;
        float attenuation = 1.0;
        float cone = 1.0;

        if (L.type == 1u) {
            // Directional: points toward the scene
            Ldir = normalize(-L.direction);
        } else {
            // Point or Spot
            vec3 vecToLight = L.position - worldPos.xyz;
            float dist = length(vecToLight);
            if (L.range > 0.0 && dist > L.range) {
                accum += ambientTerm; // out of range, keep ambient only
                continue;
            }
            Ldir = normalize(vecToLight);
            attenuation = computeAttenuation(dist, L.attConst, L.attLinear, L.attQuadratic);
            if (L.type == 2u) {
                cone = spotFactor(Ldir, L.direction, L.innerCos, L.outerCos);
            }
        }

        float NdotL = max(dot(N, Ldir), 0.0);
        vec3 diffuse = NdotL * inColor * L.color;

        // Blinn-Phong specular
        vec3 H = normalize(Ldir + V);
        float NdotH = max(dot(N, H), 0.0);
        float specPow = pow(NdotH, max(lighting.shininess, 1.0));
        vec3 specular = L.specular * specPow * L.color;

        accum += (ambientTerm + diffuse + specular) * attenuation * cone;
    }

    fragColor    = clamp(accum, 0.0, 1.0);
    fragTexCoord = inTexCoord;
    fragNormal   = N;
    fragWorldPos = worldPos.xyz;
}