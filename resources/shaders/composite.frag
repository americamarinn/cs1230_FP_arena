#version 330 core
out vec4 FragColor;
in vec2 uv;

uniform sampler2D scene;
uniform sampler2D bloomBlur;
uniform float exposure; // <--- Make sure this is sent as 1.0 in C++

void main() {
    vec3 color = texture(scene, uv).rgb;
    // Simple pass-through for start screen
    FragColor = vec4(color, 1.0);
}

// #version 330 core
// out vec4 FragColor;
// in vec2 uv;

// uniform sampler2D scene;
// uniform sampler2D bloomBlur;

// void main() {
//     vec3 hdrColor = texture(scene, uv).rgb;
//     vec3 bloomColor = texture(bloomBlur, uv).rgb;
    
//     // Additive blending
//     vec3 result = hdrColor + bloomColor;
    
//     // Tone mapping (optional, prevents colors washing out to white)
//     // result = vec3(1.0) - exp(-result * 1.0);

//     FragColor = vec4(result, 1.0);
// }
