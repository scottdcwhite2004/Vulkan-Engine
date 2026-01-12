#version 450

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inColor;     // added
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inNormal;    // added (not used, but now consumed)

layout(set = 0, binding = 0) uniform Matrices {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

layout(location = 0) out vec3 vColor;
layout(location = 1) out vec2 fragTexCoord;

void main() {
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPos, 1.0);
    fragTexCoord = inTexCoord;
    vColor = inColor; // consume location 1
    // inNormal (location 3) is present but not needed in this shader
}