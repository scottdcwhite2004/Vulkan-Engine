#version 450

layout(location = 0) in vec3 inPos;

layout(set = 0, binding = 0) uniform UBO {
    mat4 model;
    mat4 view; // unused in shadow pass
    mat4 proj; // unused in shadow pass
} ubo;

layout(set = 1, binding = 1) uniform ShadowUBO {
    mat4 lightView;
    mat4 lightProj;
} shadowUBO;

void main() {
    vec4 worldPos = ubo.model * vec4(inPos, 1.0);

    // Position in light clip-space for shadow map depth
    gl_Position = shadowUBO.lightProj * shadowUBO.lightView * worldPos;
}