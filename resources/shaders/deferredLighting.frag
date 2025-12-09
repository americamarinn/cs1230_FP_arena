// Replace the ENTIRE content of deferredLighting.frag with this
#version 330 core
out vec4 fragColor;

in vec2 uv; // Input from fullscreen_quad.vert

// Samplers for the G-Buffer textures
uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gAlbedo;
uniform sampler2D gEmissive;

// -------- Global coefficients (match C++ names) --------
uniform float k_a;
uniform float k_d;
uniform float k_s;

uniform vec3 camPos;

// light description (Copied from default.frag)
struct Light {
    int   type;      // 0 = point, 1 = directional, 2 = spot
    vec3  color;
    vec3  pos;
    vec3  dir;
    vec3  atten;
    float angle;
    float penumbra;  // outer - inner
};

uniform int   numLights;
uniform Light lights[8];

// ----------------------------------------------------
// 🛠️ DEBUG SWITCH: Change this value to visualize a buffer
// 0 = Full Lighting, 1 = Position, 2 = Normal, 3 = Albedo, 4 = Emissive
// ----------------------------------------------------
#define DEBUG_VIEW 0 // <-- Set to 0 for full lighting

// distanceFalloff (Copied from default.frag)
float distanceFalloff(vec3 coeffs, float d) {
    float a = coeffs.x;
    float b = coeffs.y;
    float c = coeffs.z;

    float denom = a + d * (b + c * d);
    float att = 1.0 / max(denom, 1e-6);
    return min(1.0, att);
}

// spotFalloff (Copied from default.frag)
float spotFalloff(float angleToAxis, float angle, float penumbra) {
    if (angleToAxis >= angle) {
        return 0.0;
    }

    if (penumbra <= 0.0) {
        return 1.0;
    }

    float t = angleToAxis - (angle - penumbra);
    return t > 0.0 ? pow(1.0 - t/penumbra, 2.0) : 1.0;
}

// Light contribution function (Adapted from default.frag)
vec3 lightContrib(Light light, vec3 wsPosition, vec3 wsNormal, vec3 camPos, vec3 albedoColor) {
    float shininess = 32.0; // Placeholder shininess
    vec3 cSpecular = vec3(1.0); // Placeholder white specular color

    vec3 N = normalize(wsNormal);
    vec3 V = normalize(camPos - wsPosition);

    vec3 L;
    float d = 0.0;
    float attenuation = 1.0;

    if (light.type == 0 || light.type == 2) { // Point or Spot
        L = light.pos - wsPosition;
        d = length(L);
        L = normalize(L);

        attenuation = distanceFalloff(light.atten, d);

        if (light.type == 2) { // Spot
            float angleToAxis = acos(dot(-L, normalize(light.dir)));
            attenuation *= spotFalloff(angleToAxis, light.angle, light.penumbra);
        }

    } else if (light.type == 1) { // Directional
        L = normalize(light.dir);
        attenuation = 1.0; // No falloff for directional
    }

    float NdotL = max(dot(N, L), 0.0);

    // if light behind the surface
    if (NdotL <= 0.0) {
        return vec3(0.0);
    }

    // diffuse
    vec3 diffuse  = k_d * albedoColor * NdotL * light.color;

    // specular
    vec3 specular = vec3(0.0);
    if (shininess > 0.0 && k_s > 0.0) {
        vec3 R      = reflect(-L, N);
        float RdotV = max(dot(R, V), 0.0);
        float sTerm = pow(RdotV, shininess);
        specular    = k_s * cSpecular * sTerm * light.color;
    }

    // each light will return its local phong contribution in RGB
    return attenuation * (diffuse + specular);
}


void main() {
    // Read data from the G-Buffer textures
    vec3 position  = texture(gPosition, uv).rgb;
    vec3 normal    = texture(gNormal, uv).rgb;
    vec4 albedo    = texture(gAlbedo, uv);
    vec3 emissive  = texture(gEmissive, uv).rgb;

    vec3 debug_color = vec3(0.0);

#if DEBUG_VIEW == 1
    // 1. Position Buffer (World-Space)
    debug_color = position * 0.5 + 0.5; // Map [-X, X] range to [0, 1] for color
#elif DEBUG_VIEW == 2
    // 2. Normal Buffer (World-Space)
    debug_color = normal * 0.5 + 0.5;   // Map [-1, 1] range to [0, 1] for color
#elif DEBUG_VIEW == 3
    // 3. Albedo Buffer
    debug_color = albedo.rgb;
#elif DEBUG_VIEW == 4
    // 4. Emissive Buffer
    debug_color = emissive;
#else
    // 0. Full Lighting Pass

    // Use Albedo color as ambient/diffuse material color
    vec3 albedoColor = albedo.rgb;

    // Ambient term
    vec3 final_color = k_a * albedoColor;

    // Lights
    int count = min(numLights, 8);
    for (int i = 0; i < count; ++i) {
        final_color += lightContrib(lights[i], position, normal, camPos, albedoColor);
    }

    // Add Emissive
    final_color += emissive;

    debug_color = final_color;
#endif

    fragColor = vec4(debug_color, 1.0);
    // fragColor = vec4(1.0, 0.0, 1.0, 1.0);
}
