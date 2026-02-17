#version 450
#extension GL_GOOGLE_include_directive : enable

layout(location = 0) in vec3 inPosition;

#define UBO_LIGHT_NAME light
#include "ParameterUBO.glsl"
#include "TerrainField.glsl"

layout(location = 0) out vec3 outWorldPos;

void main() {
    vec2 p = inPosition.xy;
    float height = terrain_height(p);

    float baseSurfaceZ = ubo.waterRules.w;
    float surfaceEpsilon = max(ubo.cellSize * 0.25f, 0.001f);
    float applyDisplacement = step(abs(inPosition.z - baseSurfaceZ), surfaceEpsilon);

    vec4 localPosition = vec4(inPosition.xyz, 1.0f);
    localPosition.z += height * applyDisplacement;

    vec4 worldPosition = ubo.model * localPosition;
    outWorldPos = worldPosition.xyz;
    gl_Position = ubo.projection * ubo.view * worldPosition;
}
