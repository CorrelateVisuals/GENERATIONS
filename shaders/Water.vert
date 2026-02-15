#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;

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
    fragColor = vec4(0.14f, 0.23f, 0.28f, 1.0f);
    gl_Position = projection * viewPosition;
}

