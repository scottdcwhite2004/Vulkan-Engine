#version 450

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNormal;

layout(set = 0, binding = 0) uniform UBO {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

layout(location = 0) out vec3 vNormalWS;
layout(location = 1) out vec3 vViewDirWS;

void main()
{
    vec4 worldPos = ubo.model * vec4(inPos, 1.0);
    // Normal matrix
    mat3 nrm = mat3(transpose(inverse(ubo.model)));
    vNormalWS = normalize(nrm * inNormal);

    vec3 camPosWS = inverse(ubo.view)[3].xyz;
    vViewDirWS = normalize(camPosWS - worldPos.xyz);

    gl_Position = ubo.proj * ubo.view * worldPos;
}