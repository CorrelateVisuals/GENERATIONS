#version 450
#extension GL_GOOGLE_include_directive : enable

layout(location = 0) in vec4 inPosition;
layout(location = 1) in vec3 inVertex;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec4 inColor;
layout(location = 4) in ivec4 inStates;

#define UBO_LIGHT_NAME light
#include "ParameterUBO.glsl"
#include "TerrainField.glsl"

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

    // Moving cells: follow inPosition.xy from Engine.comp, fixed travel size
    vec3 followerBase = inPosition.xyz;
    float followerScale = ubo.cellSize * 0.45f;

    float cellLift = max(followerScale * 0.52f, 0.08f);
    float followerCenterLift = cellLift;
    followerBase.z += terrain_height(inPosition.xy) + followerCenterLift;

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

    vec3 localNormal = safe_normalize(inNormal.xyz, vec3(0.0f, 0.0f, 1.0f));
    vec3 fixedLightDir = safe_normalize(vec3(0.35f, 0.45f, 0.82f), vec3(0.0f, 0.0f, 1.0f));
    float ndotl = max(dot(localNormal, fixedLightDir), 0.0f);
    float stableShade = 0.28f + ndotl * 0.72f;

    vec3 baseColor = clamp(inColor.rgb * 0.90f + vec3(0.10f), 0.0f, 1.0f);
    fragColor = vec4(clamp(baseColor * stableShade, 0.0f, 1.0f), inColor.a);
    gl_Position = ubo.projection * viewPosition;
}
