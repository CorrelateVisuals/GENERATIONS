#version 450

layout(location = 0) in vec3 inWorldPos;
layout(location = 1) in vec3 inWorldNormal;

layout (binding = 0) uniform ParameterUBO {
    vec4 light;
    ivec2 gridXY;
    float waterThreshold;
    float cellSize;
    vec4 waterRules;
    mat4 model;
    mat4 view;
    mat4 projection;
} ubo;

layout(binding = 3) uniform sampler2D texSampler;

layout(location = 0) out vec4 outColor;

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
    vec3 sand   = vec3(0.72f, 0.66f, 0.50f);
    vec3 grassA = vec3(0.46f, 0.58f, 0.41f);
    vec3 grassB = vec3(0.34f, 0.47f, 0.36f);
    vec3 rock   = vec3(0.43f, 0.42f, 0.44f);
    vec3 snow   = vec3(0.80f, 0.82f, 0.84f);

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

    vec3 land = pixelTerrainPalette(heightFromWater);
    vec3 rock = vec3(0.43f, 0.42f, 0.44f);
    float rockMix = smoothstep(0.28f, 0.78f, slope);
    land = mix(land, rock, rockMix * 0.7f);

    float detail = noise2(xr * 0.18f) * 0.6f + noise2(xr * 0.72f) * 0.4f;
    land *= 0.92f + detail * 0.16f;

    vec3 deepWater = vec3(0.15f, 0.24f, 0.29f);
    vec3 shallowWater = vec3(0.24f, 0.33f, 0.37f);
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
    vec3 dx = dFdx(inWorldPos);
    vec3 dy = dFdy(inWorldPos);
    vec3 normal = normalize(cross(dx, dy));
    if (normal.z < 0.0f) {
        normal = -normal;
    }
    vec3 lightDirection = normalize(ubo.light.rgb - inWorldPos);
    float heightFromWater = inWorldPos.z - ubo.waterThreshold;
    float slope = 1.0f - clamp(normal.z, 0.0f, 1.0f);
    vec3 albedo = terrainColor(heightFromWater, slope, inWorldPos.xz);

    const float ambientStrength = 0.34f;
    float diffuse = max(dot(normal, lightDirection), 0.0f);
    vec3 lit = albedo * (ambientStrength + diffuse) * 0.96f;
    outColor = vec4(lit, 1.0f);
}
