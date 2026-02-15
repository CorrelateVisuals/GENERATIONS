#version 450
#extension GL_GOOGLE_include_directive : enable

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;

#define UBO_LIGHT_NAME light
#include "ParameterUBO.glsl"
vec4 light = ubo.light;
ivec2 gridXY = ubo.gridXY;
mat4 model = ubo.model;
mat4 view = ubo.view;
mat4 projection = ubo.projection;
float waterThreshold = ubo.waterThreshold;

float waterSurfaceOffset = max(0.08f, ubo.cellSize * 0.9f);
vec2 gridSpan = max(vec2(gridXY) - vec2(1.0f), vec2(1.0f));
vec2 terrainMin = -0.5f * gridSpan;
vec2 terrainXY = terrainMin + vec2(inTexCoord.x, 1.0f - inTexCoord.y) * gridSpan;
vec4 position = vec4(terrainXY, waterThreshold + waterSurfaceOffset, 1.0f);
vec4 worldPosition = model * position;
vec4 viewPosition = view * worldPosition;

layout(location = 0) out vec4 fragColor;

void main() {
    fragColor = vec4(0.12f, 0.20f, 0.30f, 1.0f);
    gl_Position = projection * viewPosition;
}

