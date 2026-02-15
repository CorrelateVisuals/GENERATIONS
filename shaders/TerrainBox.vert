#version 450

layout(location = 0) in vec3 inPosition;

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

layout(location = 0) out vec3 outWorldPos;

void main() {
    vec4 worldPosition = ubo.model * vec4(inPosition, 1.0f);
    outWorldPos = worldPosition.xyz;
    gl_Position = ubo.projection * ubo.view * worldPosition;
}
