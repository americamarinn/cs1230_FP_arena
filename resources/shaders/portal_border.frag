#version 330 core

uniform vec3 borderColor;

out vec4 fragColor;

void main(void) {
    fragColor = vec4(borderColor, 1.0f);
}
