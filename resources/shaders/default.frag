#version 330 core

// Inputs from vertex shader (world-space)
in vec3 wsPosition;
in vec3 wsNormal;

// NEW: object-space position for blocky effect
in vec3 localPos;

// Output
out vec4 fragColor;

// -------- Global coefficients --------
uniform float k_a;
uniform float k_d;
uniform float k_s;

// -------- Material properties (default) --------
uniform vec3  cAmbient;
uniform vec3  cDiffuse;
uniform vec3  cSpecular;
uniform float shininess;

// camera world space
uniform vec3 camPos;

// light description
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

// NEW: 0 = normal shading, 1 = apply blocky “margin” effect
uniform int useBlocky;

// === NEW: normal-mapped brick path uniforms ===
uniform int   usePathMaterial;    // 1 = this fragment is a path brick
uniform int   useNormalMap;       // 1 = actually use normal map
uniform float pathUVScale;        // how much to tile bricks
uniform sampler2D pathDiffuseMap; // brick color
uniform sampler2D pathNormalMap;  // brick normal (tangent space)


// === NEW: grass bump-mapped terrain uniforms ===
uniform int   useGrassBump;        // 1 = current fragment is grass terrain
uniform sampler2D grassDiffuseMap; // grass color
uniform sampler2D grassHeightMap;  // height/bump map
uniform float grassUVScale;        // tiling
uniform float grassBumpScale;      // bump strength


// 1 / (a + b d + c d^2), clamped to [0,1]
float distanceFalloff(vec3 coeffs, float d) {
    float a = coeffs.x;
    float b = coeffs.y;
    float c = coeffs.z;

    float denom = a + d * (b + c * d);
    float att = 1.0 / max(denom, 1e-6);
    return min(1.0, att);
}


// Smooth spotlight falloff between inner & outer angle
float spotFalloff(float angleToAxis, float outerAngle, float penumbra) {
    float inner = outerAngle - penumbra;
    float outer = outerAngle;

    if (angleToAxis <= inner) return 1.0;
    if (angleToAxis >= outer) return 0.0;

    float t  = (angleToAxis - inner) / (outer - inner);
    float falloff = 3.0 * t * t - 2.0 * t * t * t;
    return 1.0 - falloff;
}

// Diffuse + specular from one light
// NEW: takes material diffuse/specular as parameters
vec3 shadeOneLight(Light light, vec3 N, vec3 P, vec3 V,
                   vec3 matDiffuse, vec3 matSpecular) {
    vec3 L;
    float attenuation = 1.0;

    if (light.type == 0) {
        // point
        vec3 disp = light.pos - P;
        float dist = length(disp);
        L = disp / dist;
        attenuation = distanceFalloff(light.atten, dist);
    } else if (light.type == 1) {
        // directional
        L = normalize(-light.dir);
        attenuation = 1.0;
    } else {
        // spot
        vec3 disp = light.pos - P;
        float dist = length(disp);
        L = disp / dist;

        attenuation = distanceFalloff(light.atten, dist);

        float angleToAxis = acos(dot(-L, normalize(light.dir)));
        attenuation *= spotFalloff(angleToAxis, light.angle, light.penumbra);
    }

    float NdotL = max(dot(N, L), 0.0);
    if (NdotL <= 0.0) {
        return vec3(0.0);
    }

    // diffuse
    vec3 diffuse  = k_d * matDiffuse * NdotL * light.color;

    // specular
    vec3 specular = vec3(0.0);
    if (shininess > 0.0 && k_s > 0.0) {
        vec3 R      = reflect(-L, N);
        float RdotV = max(dot(R, V), 0.0);
        float sTerm = pow(RdotV, shininess);
        specular    = k_s * matSpecular * sTerm * light.color;
    }

    return attenuation * (diffuse + specular);
}

// ---- Blocky margin effect for cubes ----
vec3 applyBlockyMargin(vec3 baseColor) {
    // Decide which face we’re on from the normal
    vec3 N = normalize(wsNormal);
    vec3 aN = abs(N);
    vec2 uv;

    // We assume cube in [-0.5, 0.5]^3, so we pick 2D coords per face.
    if (aN.x >= aN.y && aN.x >= aN.z) {
        // +/- X face: use (y,z)
        uv = localPos.yz;
    } else if (aN.y >= aN.x && aN.y >= aN.z) {
        // +/- Y face: use (x,z)
        uv = localPos.xz;
    } else {
        // +/- Z face: use (x,y)
        uv = localPos.xy;
    }

    // Map from [-0.5, 0.5] -> [0,1]
    uv = uv + vec2(0.5);

    float margin = 0.12;

    float insideX = step(margin, uv.x) * step(margin, 1.0 - uv.x);
    float insideY = step(margin, uv.y) * step(margin, 1.0 - uv.y);
    float inside  = insideX * insideY;

    vec3 darker = baseColor * 0.6;
    return mix(darker, baseColor, inside);
}

// MAIN

void main() {
    // Base normal from geometry
    vec3 N = normalize(wsNormal);
    vec3 V = normalize(camPos - wsPosition);

    // Default material (non-path cubes, terrain, snake, etc.)
    vec3 matDiffuse  = cDiffuse;
    vec3 matSpecular = cSpecular;

    // ====== GRASS BUMP-MAPPED TERRAIN ======
    if (useGrassBump == 1) {
        // Use XZ as UVs
        vec2 uv = wsPosition.xz * grassUVScale;

        // Grass base color
        vec3 grassColor = texture(grassDiffuseMap, uv).rgb;
        matDiffuse = grassColor;

        // --- BUMP MAPPING ---

        // size of one texel in UV space
        ivec2 size = textureSize(grassHeightMap, 0);
        vec2 texel = 1.0 / vec2(size);

        // heights
        float hC = texture(grassHeightMap, uv).r;
        float hX = texture(grassHeightMap, uv + vec2(texel.x, 0.0)).r;
        float hZ = texture(grassHeightMap, uv + vec2(0.0, texel.y)).r;

        // slopes (scaled)
        float dHdX = (hX - hC) * grassBumpScale;
        float dHdZ = (hZ - hC) * grassBumpScale;

        // terrain lies on the XZ plane with up = +Y
        // so we can build a bumped world-space normal directly
        vec3 bumpedN = vec3(-dHdX, 1.0, -dHdZ);
        N = normalize(bumpedN);
    }


    // ====== PATH BRICK MATERIAL (diffuse + optional normal map) ======
    else if (usePathMaterial == 1) {
        // UV: tile over world XZ, so path looks like repeating bricks
        vec2 uv = wsPosition.xz * pathUVScale;

        // Sample brick albedo
        vec3 brickColor = texture(pathDiffuseMap, uv).rgb;
        matDiffuse = brickColor;

        if (useNormalMap == 1) {
            // Normal from normal map (tangent space)
            vec3 nTex = texture(pathNormalMap, uv).rgb;
            nTex = normalize(nTex * 2.0 - 1.0); // [0,1] -> [-1,1]

            // Approx tangent frame for a flat y-up surface
            vec3 T = vec3(1.0, 0.0, 0.0);
            vec3 B = vec3(0.0, 0.0, 1.0);
            vec3 Nbase = vec3(0.0, 1.0, 0.0);

            mat3 TBN = mat3(T, B, Nbase);
            N = normalize(TBN * nTex);
        }
    }

    // Ambient term uses (possibly overridden) diffuse color
    vec3 color = k_a * matDiffuse;

    // lights
    int count = min(numLights, 8);
    for (int i = 0; i < count; ++i) {
        color += shadeOneLight(lights[i], N, wsPosition, V,
                               matDiffuse, matSpecular);
    }

    // Apply blocky face margins only when enabled
    if (useBlocky == 1) {
        color = applyBlockyMargin(color);
    }

    color = clamp(color, 0.0, 1.0);
    fragColor = vec4(color, 1.0);
}


// void main() {
//     // Flat debug color
//     fragColor = vec4(1.0, 0.2, 0.2, 1.0);
//     // Or: normals
//     // fragColor = vec4(abs(normalize(wsNormal)), 1.0);
// }
