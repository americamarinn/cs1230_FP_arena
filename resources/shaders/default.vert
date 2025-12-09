#version 330 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 proj;

// Match the fragment shader inputs:
out vec3 wsPosition;
out vec3 wsNormal;

// NEW: local (object-space) position for blocky borders
out vec3 localPos;

void main() {
    // World-space position
    vec4 worldPosition = model * vec4(position, 1.0);
    wsPosition = worldPosition.xyz;

    // World-space normal
    wsNormal = mat3(model) * normal;

    // Pass along the object-space position (cube in [-0.5,0.5]^3)
    localPos = position;

    // Clip-space position
    gl_Position = proj * view * worldPosition;
}
