#version 450

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;   // unused here
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inNormal;

layout(location = 0) out vec3 vWorldPos;
layout(location = 1) out vec3 vWorldNormal;
layout(location = 2) out vec2 vTexCoord;

void main() {
    vec4 worldPos = ubo.model * vec4(inPosition, 1.0);
    vWorldPos = worldPos.xyz;

    mat3 normalMatrix = transpose(inverse(mat3(ubo.model)));
    vWorldNormal = normalize(normalMatrix * inNormal);

    vTexCoord = inTexCoord;

    gl_Position = ubo.proj * ubo.view * worldPos;
}