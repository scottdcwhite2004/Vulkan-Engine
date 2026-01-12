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

layout(set = 0, binding = 1) uniform sampler2D uTexture;

layout(location = 0) in vec3 fragColor;     // not used here; kept for interface compatibility
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragNormal;    // world-space
layout(location = 3) in vec3 fragWorldPos;  // world-space

layout(location = 0) out vec4 outColor;

float computeAttenuation(float dist, float k0, float k1, float k2) {
    return 1.0 / max(k0 + k1 * dist + k2 * dist * dist, 1e-5);
}

float spotFactor(vec3 Ldir, vec3 spotDir, float innerCos, float outerCos) {
    float c = dot(normalize(-Ldir), normalize(spotDir));
    return smoothstep(outerCos, innerCos, c);
}

void main() {
    // Material inputs
    vec4 tex = texture(uTexture, fragTexCoord);
    vec3 albedo = tex.rgb;
    float thickness = clamp(tex.a, 0.0, 1.0); // use texture alpha as thickness

    // Tunables (could be moved to a UBO/push constants if desired)
    const float TRANSLUCENCY = 0.6;   // how transmissive the material is
    const float SCATTER_POW  = 2.0;   // higher -> tighter forward scattering
    const float F0           = 0.02;  // base reflectance for dielectrics

    vec3 N = normalize(fragNormal);
    vec3 V = normalize(lighting.viewPosWorld - fragWorldPos);

    vec3 accumFront = vec3(0.0);
    vec3 accumTrans = vec3(0.0);

    int count = clamp(lighting.lightCount, 0, MAX_LIGHTS);
    for (int i = 0; i < count; ++i) {
        GPULight L = lighting.lights[i];

        vec3 Ldir;
        float attenuation = 1.0;
        float cone = 1.0;

        if (L.type == 1u) {
            // Directional
            Ldir = normalize(-L.direction);
        } else {
            // Point/Spot
            vec3 toLight = L.position - fragWorldPos;
            float dist = length(toLight);
            if (L.range > 0.0 && dist > L.range) {
                // Only ambient contributes outside the range
                accumFront += (L.ambient * albedo);
                continue;
            }
            Ldir = normalize(toLight);
            attenuation = computeAttenuation(dist, L.attConst, L.attLinear, L.attQuadratic);
            if (L.type == 2u) {
                cone = spotFactor(Ldir, L.direction, L.innerCos, L.outerCos);
            }
        }

        // Front lighting (diffuse + specular)
        float NdotL = max(dot(N, Ldir), 0.0);
        vec3 diffuse = NdotL * albedo * L.color;

        vec3 H = normalize(Ldir + V);
        float NdotH = max(dot(N, H), 0.0);
        float specPow = pow(NdotH, max(lighting.shininess, 1.0));
        vec3 specular = L.specular * specPow * L.color;

        vec3 ambient = L.ambient * albedo;

        vec3 front = (ambient + diffuse + specular) * attenuation * cone;
        accumFront += front;

        // Transmission/back-lighting term: use opposite normal to simulate light passing through
        float back = pow(max(dot(-N, Ldir), 0.0), SCATTER_POW);
        vec3 transColor = albedo * L.color; // tint transmission by albedo and light color
        vec3 trans = back * transColor * attenuation * cone;

        accumTrans += trans;
    }

    // Fresnel to preserve specular at grazing angles; bias transmission by (1 - F)
    float NdotV = max(dot(N, V), 0.0);
    float F = F0 + (1.0 - F0) * pow(1.0 - NdotV, 5.0);

    float tWeight = clamp(TRANSLUCENCY * thickness * (1.0 - F), 0.0, 1.0);
    vec3 color = mix(accumFront, accumTrans, tWeight);

    // Alpha drives blending in the pipeline
    float alpha = tWeight;
    outColor = vec4(color, alpha);
}