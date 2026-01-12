#version 450

// set 0 binding 0 : per-object UBO (model, view, proj)
layout(std140, set = 0, binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view; // unused here
    mat4 proj; // unused here
} ubo;

// set 1 binding 0 : shadow/light matrices (lightView, lightProj)
layout(std140, set = 1, binding = 0) uniform ShadowUBO {
    mat4 lightView;
    mat4 lightProj;
} shadowUBO;

// vertex input: location 0 = position (matches Vertex::pos)
layout(location = 0) in vec3 inPos;

void main() {
    // transform to world then to light clip space
    vec4 worldPos = ubo.model * vec4(inPos, 1.0);
    gl_Position = shadowUBO.lightProj * shadowUBO.lightView * worldPos;
}