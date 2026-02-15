#version 450
#extension GL_GOOGLE_include_directive : enable

layout(location = 0) in vec4 inPosition;
layout(location = 1) in vec3 inVertex;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec4 inColor;
layout(location = 4) in ivec4 inStates;

#define UBO_LIGHT_NAME light
#include "ParameterUBO.glsl"

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

layout(location = 0) out vec4 fragColor;

vec3 safe_normalize(vec3 v, vec3 fallback) {
    float len2 = dot(v, v);
    if (!(len2 > 1e-12f)) {
        return fallback;
    }
    return v * inversesqrt(len2);
}

bool bad_vec3(vec3 v) {
    return isnan(v.x) || isnan(v.y) || isnan(v.z) ||
           isinf(v.x) || isinf(v.y) || isinf(v.z);
}

bool bad_vec4(vec4 v) {
    return isnan(v.x) || isnan(v.y) || isnan(v.z) || isnan(v.w) ||
           isinf(v.x) || isinf(v.y) || isinf(v.z) || isinf(v.w);
}

void main() {
    bool aliveCell = (inStates.x == 1);
    if (!aliveCell) {
        fragColor = vec4(0.0f);
        gl_Position = vec4(2.0f, 2.0f, 2.0f, 1.0f);
        return;
    }

    if (terrain_height(inPosition.xy) <= ubo.waterThreshold + ubo.waterRules.x) {
        fragColor = vec4(0.0f);
        gl_Position = vec4(2.0f, 2.0f, 2.0f, 1.0f);
        return;
    }

    vec3 followerBase = inPosition.xyz;
    float baseScale = max(inPosition.w * 1.20f, ubo.cellSize * 0.85f);
    float followerScale = baseScale * 0.5f;
    float lift = max(followerScale * 1.05f, 0.22f);
    float separation = max(ubo.cellSize * 0.06f, 0.02f);
    followerBase.z += terrain_height(inPosition.xy) + lift + separation;

    vec4 position = vec4(followerBase + (inVertex.xyz * followerScale), 1.0f);
#ifdef CE_DEBUG_ENABLE_CELL_INSTANCE_VERTEX_SANITIZATION_GUARDS
    if (bad_vec3(followerBase) || bad_vec3(inVertex) || bad_vec4(position) || bad_vec3(inNormal)) {
        fragColor = vec4(0.0f);
        gl_Position = vec4(2.0f, 2.0f, 2.0f, 1.0f);
        return;
    }
#endif
    vec4 worldPosition = ubo.model * position;
#ifdef CE_DEBUG_ENABLE_CELL_INSTANCE_VERTEX_SANITIZATION_GUARDS
    if (bad_vec4(worldPosition)) {
        fragColor = vec4(0.0f);
        gl_Position = vec4(2.0f, 2.0f, 2.0f, 1.0f);
        return;
    }
#endif
    vec4 viewPosition = ubo.view * worldPosition;
    vec3 worldNormal = safe_normalize(mat3(ubo.model) * inNormal.xyz, vec3(0.0f, 0.0f, 1.0f));

    vec3 lightDirection = safe_normalize(ubo.light.rgb - worldPosition.xyz, vec3(0.0f, 0.0f, 1.0f));
    float diffuse = max(dot(worldNormal, lightDirection), 0.0f);

    float bandedDiffuse = 0.95f;
    if (diffuse < 0.25f) {
        bandedDiffuse = 0.18f;
    } else if (diffuse < 0.65f) {
        bandedDiffuse = 0.45f;
    }

    float verticalFaceBoost = (1.0f - abs(worldNormal.z)) * 0.14f;
    float lighting = min(bandedDiffuse + verticalFaceBoost, 1.0f);

    fragColor = vec4(clamp(inColor.rgb * lighting, 0.0f, 1.0f), inColor.a);
    gl_Position = ubo.projection * viewPosition;
}
