#version 450

const int MAX_LIGHTS = 8;

layout(set = 0, binding = 1) uniform sampler2D uTexture;

// Shadow UBO + sampler in set 1:
// binding 0 = light matrices, binding 1 = depth sampler (compare sampler)
layout(std140, set = 1, binding = 0) uniform ShadowUBO {
    mat4 lightView;
    mat4 lightProj;
} shadowUBO;
layout(set = 1, binding = 1) uniform sampler2DShadow uShadowMap;

layout(std140, set = 0, binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

// Match CPU GPULightCPU field order (std140 will pad vec3 to 16-byte boundaries)
struct GPULight {
    // Vectors first (each vec3 behaves like a vec4 in std140 for alignment/size)
    vec3 position;
    vec3 direction;
    vec3 color;

    // Scalars after vectors (match CPU order exactly)
    uint  type;
    float ambient;
    float specular;

    float innerCos;
    float outerCos;
    float range;

    float attConst;
    float attLinear;
    float attQuadratic;

    // Optional pad to keep multiple of 16 if CPU included it; safe to keep for size match
    float pad0;
};

// Match CPU LightingUBOCPU: lights[] first, then view/shininess/lightCount/pads
layout(std140, set = 0, binding = 2) uniform LightingUBO {
    GPULight lights[MAX_LIGHTS];

    vec3  viewPosWorld; float shininess;
    int   lightCount;   uint padA; uint padB; uint padC;
} lighting;

layout(location = 0) in vec3 vWorldPos;
layout(location = 1) in vec3 vWorldNormal;
layout(location = 2) in vec2 vTexCoord;

layout(location = 0) out vec4 outColor;

float computeAttenuation(float dist, float k0, float k1, float k2) {
    return 1.0 / max(k0 + k1 * dist + k2 * dist * dist, 1e-5);
}

float smoothSpotFactor(vec3 Ldir, vec3 spotDir, float innerCos, float outerCos) {
    float c = dot(normalize(-Ldir), normalize(spotDir));
    return smoothstep(outerCos, innerCos, c);
}

void main() {
    vec3 N = normalize(vWorldNormal);
    vec3 V = normalize(lighting.viewPosWorld - vWorldPos);
    vec3 albedo = texture(uTexture, vTexCoord).rgb;

    vec3 colorAccum = vec3(0.0);
    int count = clamp(lighting.lightCount, 0, MAX_LIGHTS);

    for (int i = 0; i < count; ++i) {
        GPULight L = lighting.lights[i];

        vec3 ambientTerm = L.ambient * L.color * albedo;

        vec3 Ldir;
        float attenuation = 1.0;
        float cone = 1.0;

        bool isDirectional = (L.type == 1u);
        if (isDirectional) {
            // Directional
            Ldir = normalize(-L.direction);
        } else {
            // Point/Spot
            Ldir = (L.position - vWorldPos);
            float dist = length(Ldir);
            if (L.range > 0.0 && dist > L.range) {
                colorAccum += ambientTerm;
                continue;
            }
            Ldir = normalize(Ldir);
            attenuation = computeAttenuation(dist, L.attConst, L.attLinear, L.attQuadratic);
            if (L.type == 2u) {
                cone = smoothSpotFactor(Ldir, L.direction, L.innerCos, L.outerCos);
            }
        }

        float NdotL = max(dot(N, Ldir), 0.0);
        vec3 diffuse = NdotL * L.color * albedo;

        vec3 H = normalize(Ldir + V);
        float NdotH = max(dot(N, H), 0.0);
        float specPow = pow(NdotH, max(lighting.shininess, 1.0));
        vec3 specular = L.specular * specPow * L.color;

        // Shadow calculation (directional lights only)
        float shadow = 1.0;
        if (isDirectional) {
            // transform world pos into light clip space
            vec4 lightSpace = shadowUBO.lightProj * shadowUBO.lightView * vec4(vWorldPos, 1.0);
            // perspective divide
            lightSpace /= lightSpace.w;

            // NOTE: GLM is compiled with GLM_FORCE_DEPTH_ZERO_TO_ONE, so NDC.z is already 0..1.
            // Map X/Y from [-1,1] -> [0,1], but keep Z as-is.
            vec3 projCoords;
            projCoords.xy = lightSpace.xy * 0.5 + 0.5;
            projCoords.z  = lightSpace.z;

            // basic bias to reduce acne (can tune per-scene)
            float bias = max(0.0015, 0.005 * (1.0 - NdotL));

            // Only sample when inside light frustum; outside use lit (border sampler is white)
            if (projCoords.x >= 0.0 && projCoords.x <= 1.0 &&
                projCoords.y >= 0.0 && projCoords.y <= 1.0 &&
                projCoords.z >= 0.0 && projCoords.z <= 1.0) {
                // sampler2DShadow expects (s, t, ref). We subtract bias from the reference depth.
                shadow = texture(uShadowMap, vec3(projCoords.xy, projCoords.z - bias));
                // result: 1.0 = lit, 0.0 = in shadow (with hardware compare & linear filtering gives PCF-like)
            } else {
                shadow = 1.0;
            }
        }

        // Apply shadow only to direct lighting (diffuse + specular). Ambient remains.
        colorAccum += ambientTerm + (diffuse + specular) * shadow * attenuation * cone;
    }

    outColor = vec4(colorAccum, 1.0);
}