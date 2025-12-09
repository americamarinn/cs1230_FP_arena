#version 330 core
layout(location = 0) out vec4 gPosition;
layout(location = 1) out vec4 gNormal;
layout(location = 2) out vec4 gAlbedo;
layout(location = 3) out vec4 gEmissive;

in vec3 worldPos;
in vec3 worldNormal;

uniform vec3 albedoColor;
uniform vec3 emissiveColor;
uniform int useTexture; // 0 = Color, 1 = Texture
uniform sampler2D uTexture;

void main() {
    gPosition = vec4(worldPos, 1.0);
    gNormal = vec4(normalize(worldNormal), 1.0);

    if (useTexture == 1) {
       // Only use texture if specifically requested
       gAlbedo = texture(uTexture, vec2(worldPos.x, worldPos.z));
    } else {
       // Otherwise use the solid color passed from C++
       gAlbedo = vec4(albedoColor, 1.0);
    }

    gEmissive = vec4(emissiveColor, 1.0);
}

// // Replace the ENTIRE content of gbuffer.frag with this
// #version 330 core

// // Change vec3 to vec4 for gPosition, gNormal, and gEmissive to match GL_RGBA16F
// layout(location = 0) out vec4 gPosition;
// layout(location = 1) out vec4 gNormal;
// layout(location = 2) out vec4 gAlbedo;
// layout(location = 3) out vec4 gEmissive;

// in vec3 worldPos;
// in vec3 worldNormal;

// uniform vec3 albedo;
// uniform vec3 emissive;

// void main() {
//     // Must write a vec4 value to vec4 outputs
//     gPosition = vec4(worldPos, 1.0);
//     gNormal = vec4(normalize(worldNormal), 1.0);
//     gAlbedo = vec4(albedo, 1.0);
//     gEmissive = vec4(emissive, 1.0);
// }
