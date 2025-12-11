#version 330 core

// uniform vec3 portalColor;

out vec4 fragColor;

void main(void) {

    // front of quad is invisible, back is solid color

    // if (gl_FrontFacing) {
    //     fragColor = vec4(0.0f, 0.0f, 0.0f, 0.0f);
    // } else {
    //     fragColor = vec4(portalColor, 1.0f);
    // }

    fragColor = vec4(0.0f, 0.0f, 0.0f, 0.0f);
}
