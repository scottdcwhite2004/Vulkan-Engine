#version 450

layout(location = 0) in vec3 inNormal;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec3 inWorldPos;

layout(location = 0) out vec4 outColor;

// Binding 0: UBO (model/view/proj) — matches your C++ UniformBufferObject
layout(set = 0, binding = 0, std140) uniform UBO {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

// Binding 3: TimeUBO — matches your C++ TimeUBO { float time; ... }
layout(set = 0, binding = 3, std140) uniform TimeUBO {
    float time;
    float pad0;
    float pad1;
    float pad2;
} uTime;

// Optional: binding 1 sampler if needed
// layout(set = 0, binding = 1) uniform sampler2D diffuseSampler;

// Optional: binding 2 lighting UBO if used
// layout(set = 0, binding = 2, std140) uniform LightingUBO {
//     // define as per your GPU struct
// } uLight;

void main()
{
    outColor = vec4(0.6, 0.75, 1.0, 0.55);
}