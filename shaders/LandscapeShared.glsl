layout(location = 0) in vec3 inPosition;

#include "TerrainField.glsl"

vec3 safe_normalize(vec3 v, vec3 fallback) {
    float len2 = dot(v, v);
    if (!(len2 > 1e-12f)) {
        return fallback;
    }
    return v * inversesqrt(len2);
}

layout(location = 0) out vec3 outWorldPos;
layout(location = 1) out vec3 outWorldNormal;

void render_landscape_vertex(mat4 model, mat4 view, mat4 projection) {
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
