#version 450

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 0) out vec3 vDir;

void main()
{
    // Remove camera translation so the cube is centered on the camera
    mat4 viewNoTrans = ubo.view;
    viewNoTrans[3] = vec4(0.0, 0.0, 0.0, viewNoTrans[3].w);

    // Direction in view space for sampling the cubemap
    vDir = mat3(viewNoTrans) * inPosition;

    // Project the cube with translation removed
    vec4 pos = vec4(inPosition, 1.0);
    gl_Position = ubo.proj * viewNoTrans * pos;

    // Push to far plane to avoid z-fighting and ensure it draws behind everything
    gl_Position.z = gl_Position.w;
}