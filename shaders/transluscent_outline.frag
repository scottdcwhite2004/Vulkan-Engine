#version 450

layout(location = 0) in vec3 vNormalWS;
layout(location = 1) in vec3 vViewDirWS;

layout(location = 0) out vec4 outColor;

// Tweak these to control the rim thickness/softness
const float OUTLINE_THICKNESS = 0.0;  // roughly how far from the silhouette the rim appears (0..1)
const float OUTLINE_SOFTNESS  = 0.0;  // smooth falloff
const vec3  OUTLINE_COLOR     = vec3(1.0, 1.0, 1.0);
const float OUTLINE_ALPHA     = 0.2;

void main()
{
    float ndv = abs(dot(normalize(vNormalWS), normalize(vViewDirWS)));
    float rim = 1.0 - ndv;

    // Keep only a thin ring near the silhouette
    float mask = smoothstep(OUTLINE_THICKNESS - OUTLINE_SOFTNESS,
                            OUTLINE_THICKNESS + OUTLINE_SOFTNESS,
                            rim);

    if (mask <= 0.001)
        discard;

    // Premultiplied alpha (your pipeline uses ONE, ONE_MINUS_SRC_ALPHA)
    float a = clamp(mask * OUTLINE_ALPHA, 0.0, 1.0);
    outColor = vec4(OUTLINE_COLOR * a, a);
}