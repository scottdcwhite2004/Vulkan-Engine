#version 450

layout(set = 1, binding = 0) uniform samplerCube uSkybox;

layout(location = 0) in vec3 vDir;
layout(location = 0) out vec4 outColor;

void main()
{
    outColor = texture(uSkybox, normalize(vDir));
}