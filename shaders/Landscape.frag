#version 450
#extension GL_GOOGLE_include_directive : enable

layout(location = 0) in vec3 inWorldPos;
layout(location = 1) in vec3 inWorldNormal;

#define UBO_LIGHT_NAME light
#include "ParameterUBO.glsl"

layout(location = 0) out vec4 outColor;

vec3 safe_normalize(vec3 v, vec3 fallback) {
    float len2 = dot(v, v);
    if (!(len2 > 1e-12f)) {
        return fallback;
    }
    return v * inversesqrt(len2);
}

vec3 sanitize_color(vec3 c, vec3 fallback) {
    bool bad = isnan(c.x) || isnan(c.y) || isnan(c.z) ||
               isinf(c.x) || isinf(c.y) || isinf(c.z);
    if (bad) {
        return fallback;
    }
    return clamp(c, 0.0f, 1.0f);
}

float quantize(float value, float levels) {
    return floor(value * levels + 0.5f) / levels;
}

float hash21(vec2 p) {
    return fract(sin(dot(p, vec2(127.1f, 311.7f))) * 43758.5453f);
}

float noise2(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    float a = hash21(i);
    float b = hash21(i + vec2(1.0f, 0.0f));
    float c = hash21(i + vec2(0.0f, 1.0f));
    float d = hash21(i + vec2(1.0f, 1.0f));
    vec2 u = f * f * (3.0f - 2.0f * f);
    return mix(mix(a, b, u.x), mix(c, d, u.x), u.y);
}

vec3 pixelTerrainPalette(float heightRelativeToWater) {
    vec3 sand   = vec3(0.80f, 0.72f, 0.45f);
    vec3 grassA = vec3(0.38f, 0.58f, 0.36f);
    vec3 grassB = vec3(0.28f, 0.45f, 0.30f);
    vec3 rock   = vec3(0.38f, 0.37f, 0.39f);
    vec3 snow   = vec3(0.85f, 0.87f, 0.90f);

    float h = clamp((heightRelativeToWater + 0.8f) / 7.0f, 0.0f, 1.0f);

    vec3 c0 = mix(sand, grassA, smoothstep(0.08f, 0.18f, h));
    vec3 c1 = mix(c0, grassB, smoothstep(0.28f, 0.48f, h));
    vec3 c2 = mix(c1, rock, smoothstep(0.56f, 0.78f, h));
    return mix(c2, snow, smoothstep(0.80f, 1.00f, h));
}

vec3 terrainColor(float heightFromWater, float slope, vec2 worldXZ) {
    const float shoreWidth = 0.6f;
    mat2 rot = mat2(0.866f, -0.5f, 0.5f, 0.866f);
    vec2 xr = rot * worldXZ;

    vec2 gridMin = (vec2(ubo.gridXY) - vec2(1.0f)) * -0.5f;
    vec2 gridMax = gridMin + vec2(ubo.gridXY) - vec2(1.0f);
    float rightEdgeDist = gridMax.x - worldXZ.x;
    float rightEdgeFade = smoothstep(0.0f, 10.0f, rightEdgeDist);

    vec3 land = pixelTerrainPalette(heightFromWater);
    vec3 rock = vec3(0.43f, 0.42f, 0.44f);
    float stabilizedSlope = mix(0.0f, slope, rightEdgeFade);
    float rockMix = smoothstep(0.28f, 0.78f, stabilizedSlope);
    land = mix(land, rock, rockMix * 0.7f);

    float edgeDistX = min(worldXZ.x - gridMin.x, gridMax.x - worldXZ.x);
    float edgeDistY = min(worldXZ.y - gridMin.y, gridMax.y - worldXZ.y);
    float edgeDist = min(edgeDistX, edgeDistY);
    float edgeFade = smoothstep(0.5f, 3.0f, edgeDist);

    float detail = noise2(xr * 0.11f);
    land *= 0.96f + (detail * 0.08f * edgeFade * rightEdgeFade);

    vec3 deepWater = vec3(0.12f, 0.20f, 0.30f);
    vec3 shallowWater = vec3(0.20f, 0.30f, 0.42f);
    float gameplayHeightFromWater = heightFromWater - ubo.waterRules.x;
    float depthToShore = clamp((-gameplayHeightFromWater + shoreWidth) / (shoreWidth * 2.0f), 0.0f, 1.0f);
    vec3 water = mix(shallowWater, deepWater, depthToShore);

    float waterBlend = (1.0f - smoothstep(-shoreWidth, 0.0f, gameplayHeightFromWater)) * 0.72f;
    vec3 color = mix(land, water, waterBlend);

    float foam = 1.0f - smoothstep(0.0f, shoreWidth * 0.7f, abs(gameplayHeightFromWater));
    color = mix(color, vec3(0.83f, 0.86f, 0.89f), foam * waterBlend * 0.28f);

    // Explicit border highlight where gameplay water is established.
    float waterBorder = 1.0f - smoothstep(0.0f, ubo.waterRules.z, abs(gameplayHeightFromWater));
    color = mix(color, vec3(0.70f, 0.78f, 0.84f), waterBorder * 0.45f);

    return color;
}

/*void main() {
    outColor = texture(texSampler, textureCoords);
}*/

void main() {
    vec3 normal = safe_normalize(inWorldNormal, vec3(0.0f, 0.0f, 1.0f));
    if (normal.z < 0.0f) {
        normal = -normal;
    }

    vec2 gridMin = (vec2(ubo.gridXY) - vec2(1.0f)) * -0.5f;
    vec2 gridMax = gridMin + vec2(ubo.gridXY) - vec2(1.0f);
    float rightEdgeDist = gridMax.x - inWorldPos.x;
    float rightEdgeFade = smoothstep(0.0f, 10.0f, rightEdgeDist);
    normal = safe_normalize(mix(vec3(0.0f, 0.0f, 1.0f), normal, rightEdgeFade),
                            vec3(0.0f, 0.0f, 1.0f));

    vec3 lightDirection = safe_normalize(ubo.light.rgb - inWorldPos, vec3(0.0f, 0.0f, 1.0f));
    float heightFromWater = inWorldPos.z - ubo.waterThreshold;
    float slope = 1.0f - clamp(normal.z, 0.0f, 1.0f);
    vec3 albedo = terrainColor(heightFromWater, slope, inWorldPos.xz);

    const float ambientStrength = 0.34f;
    float diffuse = max(dot(normal, lightDirection), 0.0f);
    float lightTerm = clamp(ambientStrength + diffuse, 0.0f, 1.25f);
    vec3 lit = sanitize_color(albedo * lightTerm * 0.96f, vec3(0.35f, 0.42f, 0.36f));
    outColor = vec4(lit, 1.0f);
}
