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

vec2 grid_base_position(uint cellIndex) {
    float startX = (float(ubo.gridXY.x) - 1.0f) * -0.5f;
    float startY = (float(ubo.gridXY.y) - 1.0f) * -0.5f;
    return vec2(startX + float(cellIndex % uint(ubo.gridXY.x)),
                startY + float(cellIndex / uint(ubo.gridXY.x)));
}

void main() {
    bool aliveCell = (inStates.x == 1);
    if (!aliveCell) {
        fragColor = vec4(0.0f);
        gl_Position = vec4(2.0f, 2.0f, 2.0f, 1.0f);
        return;
    }

    vec2 anchoredXY = grid_base_position(gl_InstanceIndex);

    if (terrain_height(anchoredXY) <= ubo.waterThreshold + ubo.waterRules.x) {
        fragColor = vec4(0.0f);
        gl_Position = vec4(2.0f, 2.0f, 2.0f, 1.0f);
        return;
    }

    vec3 cellBase = vec3(anchoredXY, inPosition.z);
    float cellScale = max(inPosition.w * 1.20f, ubo.cellSize * 0.85f);
    float lift = max(cellScale * 0.52f, 0.08f);
    cellBase.z += terrain_height(anchoredXY) + lift;

    vec4 position = vec4(cellBase + (inVertex.xyz * cellScale), 1.0f);
#ifdef CE_DEBUG_ENABLE_CELL_INSTANCE_VERTEX_SANITIZATION_GUARDS
    if (bad_vec3(cellBase) || bad_vec3(inVertex) || bad_vec4(position) || bad_vec3(inNormal)) {
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
#version 450
#extension GL_GOOGLE_include_directive : enable

layout(location = 0) in vec4 inPosition;
layout(location = 1) in vec3 inVertex;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec4 inColor;
layout(location = 4) in ivec4 inStates;

#include "TerrainField.glsl"

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

vec2 grid_base_position(uint cellIndex) {
    float startX = (float(ubo.gridXY.x) - 1.0f) * -0.5f;
    float startY = (float(ubo.gridXY.y) - 1.0f) * -0.5f;
    return vec2(startX + float(cellIndex % uint(ubo.gridXY.x)),
                startY + float(cellIndex / uint(ubo.gridXY.x)));
}

void main() {
    bool aliveCell = (inStates.x == 1);
    if (!aliveCell) {
        fragColor = vec4(0.0f);
        gl_Position = vec4(2.0f, 2.0f, 2.0f, 1.0f);
        return;
    }

    vec2 anchoredXY = grid_base_position(gl_InstanceIndex);

    if (terrain_height(anchoredXY) <= ubo.waterThreshold + ubo.waterRules.x) {
        fragColor = vec4(0.0f);
        gl_Position = vec4(2.0f, 2.0f, 2.0f, 1.0f);
        return;
    }

    vec3 cellBase = vec3(anchoredXY, inPosition.z);
    float cellScale = max(inPosition.w * 1.20f, ubo.cellSize * 0.85f);
    float lift = max(cellScale * 0.52f, 0.08f);
    cellBase.z += terrain_height(anchoredXY) + lift;

    vec4 position = vec4(cellBase + (inVertex.xyz * cellScale), 1.0f);
#ifdef CE_DEBUG_ENABLE_CELL_INSTANCE_VERTEX_SANITIZATION_GUARDS
    if (bad_vec3(cellBase) || bad_vec3(inVertex) || bad_vec4(position) || bad_vec3(inNormal)) {
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