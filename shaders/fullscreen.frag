#version 450

layout(set = 0, binding = 0) uniform sampler2D sceneTexture;
layout(set = 0, binding = 2) uniform sampler2D uMask;

layout(std140, set = 0, binding = 1) uniform PostProcessUBO {
    float time;
} ubo;

layout(location = 0) in vec2 vUV;
layout(location = 0) out vec4 outColor;

vec3 poissonBlur(sampler2D tex, vec2 uv, vec2 pixelSize, float radius)
{
    vec3 sum = vec3(0.0);
    int count = 0;
    int r = int(radius);
    for (int j = -r; j <= r; ++j)
    {
        for (int i = -r; i <= r; ++i)
        {
            vec2 offset = vec2(i, j) * pixelSize;
            sum += texture(tex, uv + offset).rgb;
            count++;
        }
    }
    return sum / float(count);
}

float animatedRadius(vec2 uv, float timeVal) {
    float amplitude = 75.0;
    float frequency = 10.0;
    float value = 0.0;
    const int octaves = 20;
    const float gain = 0.55;
    const float lacunarity = 2.2;
    for (int o = 0; o < octaves; ++o) {
        value += sin((uv.x + timeVal * 0.3) * frequency) * amplitude;
        value += cos((uv.y + timeVal * 0.3) * frequency) * amplitude;
        amplitude *= gain;
        frequency *= lacunarity;
    }
    value = abs(value);
    return clamp(4.0 + value * 0.15, 1.0, 60.0);
}

void main() {
    vec2 uv = vUV;

    vec4 color = texture(sceneTexture, uv);
    float mask = texture(uMask, uv).r;

    if (mask > 0.5) {
        vec3 original = texture(sceneTexture, uv).rgb;
        vec2 pixelSize = 1.0 / vec2(textureSize(sceneTexture, 0));

        float radius = animatedRadius(uv, ubo.time);
        vec3 blurred = poissonBlur(sceneTexture, uv, pixelSize, radius);

        vec3 fireTintLow = vec3(10.0, 1.0, 0.0);
        vec3 fireTintHigh = vec3(1.0, 1.0, 0.0);
        float t = clamp(uv.y, 0.0, 1.0);
        vec3 fireTint = mix(fireTintLow, fireTintHigh, t);
        original *= fireTint;

        if (original.r <= 0.001) {
            original = vec3(
                fireTint.x * blurred.x * 25.0,
                fireTint.y * blurred.y * 25.0,
                fireTint.z * blurred.z * 25.0
            );
        }

        original += blurred;
        original *= blurred * 3.0;

        outColor = vec4(original, 1.0);
    } else {
        outColor = color;
    }
}