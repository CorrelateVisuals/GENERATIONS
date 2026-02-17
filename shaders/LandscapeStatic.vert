#version 450
#extension GL_GOOGLE_include_directive : enable

layout(location = 0) in vec3 inPosition;

#define UBO_LIGHT_NAME light
#include "ParameterUBO.glsl"
vec4 light = ubo.light;
ivec2 gridXY = ubo.gridXY;
mat4 model = ubo.model;
mat4 view = ubo.view;
mat4 projection = ubo.projection;
float waterThreshold = ubo.waterThreshold;

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

float fbm(vec2 p) {
    float value = 0.0f;
    float amplitude = 0.5f;
    for (int i = 0; i < 5; ++i) {
        value += amplitude * noise2(p);
        p = p * 2.02f + vec2(11.5f, 7.2f);
        amplitude *= 0.5f;
    }
    return value;
}

float ridged_fbm(vec2 p) {
    float value = 0.0f;
    float amplitude = 0.5f;
    for (int i = 0; i < 5; ++i) {
        float n = noise2(p);
        float ridge = 1.0f - abs(2.0f * n - 1.0f);
        value += ridge * amplitude;
        p = p * 2.1f + vec2(9.2f, 3.4f);
        amplitude *= 0.5f;
    }
    return value;
}

float terrain_height(vec2 p) {
    mat2 rot = mat2(0.866f, -0.5f, 0.5f, 0.866f);
    vec2 pr = rot * p;
    vec2 q = pr * 0.065f;
    vec2 warp = vec2(
        fbm(q * 1.15f + vec2(4.0f, 1.7f)),
        fbm(q * 1.15f + vec2(7.2f, 3.5f)));
    q += warp * 0.75f;

    float broad = fbm(q * 0.62f) * 3.6f;
    float base = fbm(q * 1.05f) * 2.2f;
    float ridge = ridged_fbm(q * 2.0f) * 4.2f;
    float crags = pow(max(ridged_fbm(q * 4.7f), 0.0f), 1.8f) * 1.15f;
    float macro = (sin(pr.x * 0.028f) + sin(pr.y * 0.024f)) * 0.85f;
    float detail = fbm(q * 7.6f) * 0.26f;

    float mountainMask = smoothstep(0.52f, 0.80f, ridged_fbm(q * 0.95f));
    float habitableLowlands = broad + base + macro;
    float mountainRelief = ridge + crags + detail;
    float lowlandBias = -0.55f * (1.0f - mountainMask);

    return habitableLowlands + mountainRelief * mountainMask + lowlandBias + 1.35f;
}

vec3 safe_normalize(vec3 v, vec3 fallback) {
    float len2 = dot(v, v);
    if (!(len2 > 1e-12f)) {
        return fallback;
    }
    return v * inversesqrt(len2);
}

layout(location = 0) out vec3 outWorldPos;
layout(location = 1) out vec3 outWorldNormal;

void main() {
    vec2 p = inPosition.xy;
    float height = terrain_height(p);
    float baseSurfaceZ = ubo.waterRules.w;
    float surfaceEpsilon = max(ubo.cellSize * 0.25f, 0.001f);
    float applyDisplacement = step(abs(inPosition.z - baseSurfaceZ), surfaceEpsilon);

    vec4 localPosition = vec4(inPosition.xyz, 1.0f);
    localPosition.z += height * applyDisplacement;

    vec4 worldPosition = model * localPosition;
    vec4 viewPosition = view * worldPosition;

    float eps = max(0.35f * ubo.cellSize, 0.05f);
    vec2 gridMin = (vec2(ubo.gridXY) - vec2(1.0f)) * -0.5f;
    vec2 gridMax = gridMin + vec2(ubo.gridXY) - vec2(1.0f);

    vec2 pL = vec2(max(p.x - eps, gridMin.x), p.y);
    vec2 pR = vec2(min(p.x + eps, gridMax.x), p.y);
    vec2 pD = vec2(p.x, max(p.y - eps, gridMin.y));
    vec2 pU = vec2(p.x, min(p.y + eps, gridMax.y));

    float hL = terrain_height(pL);
    float hR = terrain_height(pR);
    float hD = terrain_height(pD);
    float hU = terrain_height(pU);

    float dx = max(pR.x - pL.x, 1e-4f);
    float dy = max(pU.y - pD.y, 1e-4f);
    float dHdx = (hR - hL) / dx;
    float dHdy = (hU - hD) / dy;

    vec3 terrainNormal = safe_normalize(vec3(-dHdx, -dHdy, 1.0f), vec3(0.0f, 0.0f, 1.0f));
    float rightEdgeDist = gridMax.x - p.x;
    float rightEdgeFade = smoothstep(0.0f, 10.0f, rightEdgeDist);
    terrainNormal = safe_normalize(mix(vec3(0.0f, 0.0f, 1.0f), terrainNormal, rightEdgeFade),
                                   vec3(0.0f, 0.0f, 1.0f));
    vec3 normalLocal = mix(vec3(0.0f, 0.0f, -1.0f), terrainNormal, applyDisplacement);
    vec3 worldNormal = safe_normalize(mat3(model) * normalLocal, vec3(0.0f, 0.0f, 1.0f));

    outWorldPos = worldPosition.xyz;
    outWorldNormal = worldNormal;
    gl_Position = projection * viewPosition;
}
