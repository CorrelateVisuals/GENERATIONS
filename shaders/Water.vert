#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;

layout (binding = 0) uniform ParameterUBO {
    vec4 light;
    ivec2 gridXY;
    float waterThreshold;
    float cellSize;
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

const float GEO_TILE_SIZE = 0.3f;
vec2 gridXYsize = vec2(gridXY.x, gridXY.y) * GEO_TILE_SIZE;

vec4 worldPosition = model * vec4(inPosition, 1.0f);
vec4 viewPosition = view * worldPosition;

layout(location = 0) out vec4 fragColor;

void main() {
    fragColor = vec4(0.0f, 0.2f, 1.0f, 1.0f);
    gl_Position = projection * viewPosition;
}

