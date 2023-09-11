#version 450

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

vec3 waterVertices[4] = {
    {gridXYsize.x, gridXYsize.y, waterThreshold},
    {-gridXYsize.x, gridXYsize.y, waterThreshold},
    {-gridXYsize.x, -gridXYsize.y, waterThreshold},
    {gridXYsize.x, -gridXYsize.y, waterThreshold}, 
};
const int waterIndices[6] = {
    0, 1, 2, 0, 2, 3,       // Bottom face
};

vec4 vertex = vec4(vec3(waterVertices[waterIndices[gl_VertexIndex]]), 1.0f);
vec4 worldPosition = model * vertex;
vec4 viewPosition = view * worldPosition;

layout(location = 0) out vec4 fragColor;

void main() {
    fragColor = vec4(0.0f, 0.2f, 1.0f, 1.0f);
    gl_Position = projection * viewPosition;
}

