#version 450
layout(set = 0, binding = 1) uniform sampler2D uTexture;

layout(location = 1) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

void main() {
    vec3 albedo = texture(uTexture, fragTexCoord).rgb;
    outColor = vec4(albedo, 1.0);
}