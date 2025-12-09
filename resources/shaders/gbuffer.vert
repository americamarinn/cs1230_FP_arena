#version 330 core

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNormal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 proj;

out vec3 worldPos;
out vec3 worldNormal;

void main() {
    vec4 wp = model * vec4(inPos, 1.0);
    worldPos = wp.xyz;

    worldNormal = normalize(mat3(transpose(inverse(model))) * inNormal);

    gl_Position = proj * view * wp;
}
